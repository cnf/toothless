#include "heater/heater.hpp"

#include <esp_err.h>
#include <esp_timer.h>

#include "config_mgr.hpp"
#include "funlog.h"
// #include "heater/elements/element.hpp"
#include "heater/profiles/profile_manager.hpp"
#include "helpers/box_mover.hpp"
#include "topics.hpp"

namespace toothless {

using namespace heater;

Heater::Heater() {
  // Set up Config
  _config = std::make_shared<SettingsMap>();
  _config_entries = std::make_unique<ConfigEntries>();
  _subscription =
      ps_new_subscriber(10, PS_STRLIST("sensor.temperature", topics::heater::name, topics::profile::changed));
  _limits = std::make_unique<heater::SafetyLimits>();
}

Heater::~Heater() {
  if (_element && _element->IsOn()) {
    _element->Off();
  }
  if (_subscription) {
    ps_free_subscriber(_subscription);
    _subscription = nullptr;
  }
}

bool Heater::Init() {
  // esp_log_level_set(FLOG_SHORT_FILENAME, ESP_LOG_DEBUG);

  _config_entries->insert(std::end(*_config_entries), std::begin(config_entries), std::end(config_entries));

  // Set up Profile manager
  _profile_mgr = ProfileManager::GetInstance();
  _profile_mgr->CreateDefaults();
  UpdateProfileConfigEntry();

  // Register Config
  RegisterConfig(_config_entries.get(), topics::heater::name);
  FLOG_DEBUG("Waiting for settings...");
  GetSettings(_config, topics::heater::name);

  ApplySettings();
  GetElement();

  _time_slice = 200;

  _target = std::nullopt;
  _temperature_integral = 0;
  _previous_temperature = std::numeric_limits<int32_t>::max();

  return true;
}

void Heater::Loop() {
  static uint64_t last_update = esp_timer_get_time();
  PS_PUB_NIL("watchdog.heater");  // FIXME: flesh out watchdog mechanisms
  HandleSubscriptions();
  UpdateTimer();
  if (!_element) {
    if (last_update + 10 * 1000 * 1000 > esp_timer_get_time()) {
      return;
    }
    FLOG_WARN("Heater element missing, attempting to reacquire");
    GetElement();
    last_update = esp_timer_get_time();
    if (!_element) {
      FLOG_ERROR("No heater element configured");
      return;
    }
    return;
  }

  switch (_mode) {
    case heater::Mode::kModeReflow:
      LoopProfile();
      break;
    case heater::Mode::kModeDrying:
      LoopDryer();
      break;
    case heater::Mode::kModeHeating:
      break;
    case heater::Mode::kModeCooldown:
      break;
    default:
      AssertOff();
      FLOG_ERROR("Unknown heater mode %d", _mode);
      return;
  }

  Tune();
  if (CheckSafety() != ESP_OK) {
    AssertOff();
    PS_PUB_ERR_FL(topics::heater::error, ESP_FAIL, "Safety shutdown", PS_FL_STICKY);
    FLOG_ERROR("Heater safety shutdown");
    return;
  }
  if (_power_setting > 0 && _state == heater::kStateOn) {
    HeaterOn(_power_setting);
  } else {
    if (_element->IsOn()) HeaterOff();
  }
}

esp_err_t Heater::HandleSubscriptions() {
  bool new_settings = false;
  ps_msg_t* msg = NULL;
  for ((msg = ps_get(_subscription, 0)); msg != NULL; (msg = ps_get(_subscription, 0))) {
    if (ps_has_topic(msg, "sensor.temperature.zone") && PS_IS_INT(msg)) {
      _temperature = msg->int_val;
      if (_previous_temperature == std::numeric_limits<int32_t>::max()) {
        _previous_temperature = _temperature;
      }
      _last_temp_update = esp_timer_get_time();
    } else if (ps_has_topic(msg, topics::heater::state_set)) {
      if (PS_IS_INT(msg)) {
        heater::State state = static_cast<heater::State>(msg->int_val);
        SetState(state);
      } else {
        SetState(heater::kStateOff);
        FLOG_ERROR("Invalid heater state message");
      }
    } else if (ps_has_topic(msg, topics::heater::timer_set)) {
      if (PS_IS_INT(msg)) {
        SetTimer(msg->int_val);
      } else if (PS_IS_NIL(msg)) {
        ClearTimer();
      } else {
        FLOG_ERROR("Invalid heater timer message");
      }
    } else if (ps_has_topic(msg, topics::heater::mode_set) && PS_IS_INT(msg)) {
      heater::Mode mode = static_cast<heater::Mode>(msg->int_val);
      SetMode(mode);
    } else if (ps_has_topic(msg, topics::heater::start)) {
      SetState(heater::kStateOn);
    } else if (ps_has_topic(msg, topics::heater::stop)) {
      HeaterOff();
      SetState(heater::kStateOff);
    } else if (ps_has_topic(msg, topics::heater::pause)) {
      HeaterOff();
      SetState(heater::kStatePause);
    } else if (ps_has_topic(msg, topics::heater::target_temperature_set)) {
      if (PS_IS_INT(msg)) {
        SetTarget(msg->int_val);
      } else if (PS_IS_NIL(msg)) {
        ClearTarget();
      } else {
        FLOG_ERROR("Invalid heater target temperature message");
      }
    } else if (ps_has_topic(msg, topics::heater::profile_get)) {
      if (_current_profile) {
        PS_PUB_STR_FL(topics::heater::profile, _current_profile->Name().c_str(), PS_FL_STICKY);
      } else {
        PS_PUB_NIL_FL(topics::heater::profile, PS_FL_STICKY);
      }
    } else if (ps_has_topic(msg, topics::heater::profile_set)) {
      if (PS_IS_STR(msg)) {
        LoadProfile(std::string(msg->str_val));
      } else {
        FLOG_ERROR("Invalid heater profile message");
      }
    } else if (ps_has_topic(msg, topics::profile::changed)) {
      FLOG_INFO("Profile list changed, refreshing config");
      RefreshProfileConfig();
    } else if (ps_has_topic_suffix(msg, kTopicConfigGet) && PS_IS_NIL(msg)) {
      FLOG_DEBUG("Sending heater config map");
      GetSettings(_config, topics::heater::name);
      new_settings = true;
    } else {
      FLOG_VERBOSE("unknown message : %s", msg->topic);
    }
    ps_unref_msg(msg);
  }
  if (new_settings) {
    // ApplySettings();
  }
  return ESP_OK;
}

esp_err_t Heater::ApplySettings() {
  SetMode(FromString(std::get<std::string>(_config->at("mode"))));
  LoadProfile(std::get<std::string>(_config->at("profile")));
  _maximum_temperature = std::get<int>(_config->at("max_temp")) * 100;

  _kp = std::get<double>(_config->at("pid_kp"));  // expected 0.01 - 2.0
  _ki = std::get<double>(_config->at("pid_ki"));  // expected 0.0001 - 0.01
  _kd = std::get<double>(_config->at("pid_kd"));  // expected 0.0 - 50
  return ESP_OK;
}

esp_err_t Heater::GetElement() {
  FLOG_DEBUG("Getting heater element from Peripheral Registry");
  // TODO: make this safer
  ps_msg_t* msg = PS_CALL_PTR("peripheral.actuator.zone.get", &_element, 2000);
  if (msg != NULL && PS_IS_NIL(msg)) {
    ps_unref_msg(msg);
    if (_element) {
      FLOG_INFO("Heater element set: %s", _element->Info().name);
      return ESP_OK;
    }
  } else {
    FLOG_ERROR("Failed to get heater element from Peripheral Registry");
    return ESP_ERR_NOT_FOUND;
  }
  return ESP_OK;
}

void Heater::AssertOff() {
  FLOG_ERROR("Heater ASSERT OFF called!");
  ESP_ERROR_CHECK(HeaterOff());
  if (_element && _element->IsOn()) {
    FLOG_ERROR("Heater element failed to turn off!");
  }
  _power_setting = 0;
  _target = std::nullopt;
}

esp_err_t Heater::CheckSafety() {
  // 1. Thermal runaway: temp way above target (temps in centidegrees)
  if (_target.has_value() && _temperature > _target.value() + _limits->overshoot_limit) {
    FLOG_ERROR("THERMAL RUNAWAY: %d > target+%d", _temperature / 100,
               (_target.value() + _limits->overshoot_limit) / 100);
    PS_PUB_ERR_FL(topics::heater::error, ESP_ERR_INVALID_STATE, "Thermal runaway detected", PS_FL_STICKY);
    return ESP_ERR_INVALID_STATE;
  }

  // 2. Convert rate to °C/sec for comparison
  // _rate_of_change is centidegrees per _time_slice(ms)
  // To get °C/sec: (rate * 1000) / (100 * _time_slice)
  float rate_deg_per_sec = (_rate_of_change * 1000.0f) / (100.0f * _time_slice);

  // 3. Stall detection: power high but no heating
  // TODO: use pid? make configurable? a big oven heats a lot slower than a hotplate
  if (_power_setting > 50 && rate_deg_per_sec < _limits->min_heat_rate) {
    _stall_counter++;
    if (_stall_counter > _limits->stall_timeout_ms / _time_slice) {
      FLOG_ERROR("HEATING STALL: element or sensor fault");
      PS_PUB_ERR_FL(topics::heater::error, ESP_ERR_TIMEOUT, "Heating stall detected", PS_FL_STICKY);
      return ESP_ERR_TIMEOUT;
    }
  } else {
    _stall_counter = 0;
  }

  // 4. Sensor sanity: impossible rate of change
  if (std::abs(rate_deg_per_sec) > _limits->max_heat_rate) {
    FLOG_ERROR("SENSOR FAULT: rate %.1f°C/s impossible", rate_deg_per_sec);
    PS_PUB_ERR_FL(topics::heater::error, ESP_ERR_INVALID_RESPONSE,
                  std::format("Temperature sensor fault: rate {:.1f}°C/s impossible", rate_deg_per_sec).c_str(),
                  PS_FL_STICKY);
    return ESP_ERR_INVALID_RESPONSE;
  }

  // 5. Sensor timeout: if no temp updates in sensor_timeout_ms
  if (_last_temp_update + _limits->sensor_timeout_ms * 1000 < esp_timer_get_time()) {
    FLOG_ERROR("NO TEMPERATURE UPDATES");
    PS_PUB_ERR_FL(topics::heater::error, ESP_ERR_TIMEOUT,
                  std::format("No temperature updates received for {}s", _limits->sensor_timeout_ms / 1000).c_str(),
                  PS_FL_STICKY);
    return ESP_ERR_TIMEOUT;
  }

  return ESP_OK;
}

esp_err_t Heater::LoadProfile(std::string name) {
  _current_profile = _profile_mgr->GetProfile(name);
  if (!_current_profile) {
    FLOG_ERROR("Profile '%s' not found", name.c_str());
    return ESP_ERR_NOT_FOUND;
  }
  PS_PUB_STR_FL(topics::heater::profile, name.c_str(), PS_FL_STICKY);
  return ESP_OK;
}

esp_err_t Heater::RefreshProfileConfig() {
  UpdateProfileConfigEntry();

  // Notify config system that options have changed
  PS_PUB_NIL("config.refresh.heater");  // FIXME: what is this?

  return ESP_OK;
}

std::optional<uint16_t> Heater::GetTemperature() { return std::nullopt; }

uint64_t Heater::GetTimeSecondsLeft() {
  uint32_t elapsed;
  switch (_mode) {
    case heater::Mode::kModeReflow:
      if (!_current_profile) return 0;
      elapsed = (esp_timer_get_time() / 1000 - _start_time_ms);
      return (_current_profile->TotalDuration() - elapsed) / 1000;
      break;
    case heater::Mode::kModeDrying:
      return _time_remaining_ms / 1000;
      break;
    case heater::Mode::kModeHeating:
      return 0;
      break;
    case heater::Mode::kModeCooldown:
      return 0;  // 5 minutes
      break;
    default:
      return 0;
      break;
  }
  //
  // return (_timer_ms - (esp_timer_get_time() / 1000 - _start_time)) / 1000;
}

esp_err_t Heater::SetState(heater::State state) {
  switch (state) {
    case heater::kStateOn:
      StateToOn();
      break;
    case heater::kStateOff:
      StateToOff();
      break;
    case heater::kStatePause:
      StateToPause();
      break;
    default:
      FLOG_ERROR("Unknown heater state %d", state);
      return ESP_ERR_INVALID_ARG;
  }
  return ESP_OK;
}

esp_err_t Heater::SetMode(heater::Mode new_mode) {
  switch (new_mode) {
    case heater::Mode::kModeHeating:
      _mode = heater::Mode::kModeHeating;
      // FLOG_DEBUG("Heater mode: HEATING");
      break;
    case heater::Mode::kModeDrying:
      _mode = heater::Mode::kModeDrying;
      // FLOG_DEBUG("Heater mode: DRYING");
      break;
    case heater::Mode::kModeReflow:
      _mode = heater::Mode::kModeReflow;
      // FLOG_DEBUG("Heater mode: REFLOW");
      break;
    case heater::Mode::kModeCooldown:
      _mode = heater::Mode::kModeCooldown;
      // FLOG_DEBUG("Heater mode: COOLDOWN");
      break;
    default:
      FLOG_ERROR("Unknown heater mode %d", _mode);
      return ESP_ERR_INVALID_ARG;
  }
  PS_PUB_INT_FL(topics::heater::mode, _mode, PS_FL_STICKY);
  FLOG_DEBUG("Heater mode set to %s", ToString(_mode).c_str());
  return ESP_OK;
}

esp_err_t Heater::SetPower(uint8_t power) {
  if (power > 0) {
    _element->On(power);
  } else {
    _element->Off();
  }
  return ESP_OK;
}

esp_err_t Heater::SetTimer(uint32_t time_sec) {
  _timer_ms = time_sec * 1000;  // store in ms
  _time_remaining_ms = _timer_ms;
  PS_PUB_INT_FL(topics::heater::timer, _timer_ms / 1000, PS_FL_STICKY);
  PS_PUB_INT_FL(topics::heater::timer_remaining, _time_remaining_ms / 1000, PS_FL_STICKY);
  FLOG_DEBUG("Heater timer set to %d seconds", _timer_ms / 1000);
  return ESP_OK;
}

esp_err_t Heater::ClearTimer() {
  _timer_ms = 0;
  _time_remaining_ms = 0;
  PS_PUB_NIL_FL(topics::heater::timer, PS_FL_STICKY);
  PS_PUB_NIL_FL(topics::heater::timer_remaining, PS_FL_STICKY);
  FLOG_DEBUG("Heater timer cleared");
  return ESP_OK;
}

esp_err_t Heater::SetTarget(int32_t target) {
  if (target > 99900 || target < -9900) {
    FLOG_ERROR("Target temperature %d out of range (-99 to 999)", target);
    // PS_PUB_ERR_FL(kTopicStatusError, ESP_ERR_INVALID_ARG,
    // std::format("Heater target temperature {} out of range", target).c_str(), PS_FL_STICKY);
    PS_PUB_ERR_FL(topics::heater::error, ESP_ERR_INVALID_ARG,
                  std::format("Heater target temperature {} out of range", target).c_str(), PS_FL_STICKY);
    return ESP_ERR_INVALID_ARG;
  }
  if (target > _maximum_temperature) {
    PS_PUB_STR(kTopicStatusWarning,
               std::format("Requested target temperature of {}C exceeds maximum of {}, capping to maximum",
                           target / 100, _maximum_temperature / 100)
                   .c_str());
    _target = _maximum_temperature;
  } else {
    _target = target;
  }
  if (_target.has_value()) {
    FLOG_DEBUG("Set target temperature to %d", _target.value() / 100);
    PS_PUB_INT_FL(topics::heater::target_temperature, _target.value(), PS_FL_STICKY);
  }
  return ESP_OK;
}

esp_err_t Heater::ClearTarget() {
  _target = std::nullopt;
  PS_PUB_NIL_FL(topics::heater::target_temperature, PS_FL_STICKY);
  FLOG_DEBUG("Cleared target temperature");
  return ESP_OK;
};

esp_err_t Heater::HeaterOn(float power) {
  FLOG_DEBUG("HeaterOn called with float power: %0.2f", power);
  return HeaterOn(static_cast<uint8_t>(power));
}

esp_err_t Heater::HeaterOn(uint8_t power) {
  if (!_element) {
    FLOG_WARN("Heater element missing, cannot turn on");
    return ESP_ERR_INVALID_STATE;
  }
  return _element->On(power);
}

esp_err_t Heater::HeaterOff() {
  FLOG_DEBUG("HeaterOff called");
  if (!_element) {
    FLOG_WARN("Heater element missing, cannot turn off");
    return ESP_ERR_INVALID_STATE;
  }
  return _element->Off();
}

void Heater::UpdateProfileConfigEntry() {
  for (auto& entry : *_config_entries) {
    if (std::string(entry.key) == "profile") {
      auto profile_names = _profile_mgr->ListProfiles();

      if (profile_names.empty()) {
        FLOG_WARN("No profiles available for config");
        entry.format = "enum=None";
        return;
      }

      // Build enum string with NORMALIZED names
      std::string enum_str = "enum=";
      for (size_t i = 0; i < profile_names.size(); ++i) {
        if (i > 0) enum_str += "|";
        // **NORMALIZE PROFILE NAMES IN FORMAT**

        // std::string normalized = profile_names[i];
        // std::replace(normalized.begin(), normalized.end(), ' ', '_');
        // std::transform(normalized.begin(), normalized.end(), normalized.begin(), ::tolower);
        enum_str += config_utils::NormalizeString(profile_names[i]);
      }

      entry.format = enum_str;

      // Update default value to normalized first profile if current isn't valid
      // std::string current_default = std::get<std::string>(entry.default_value);
      // std::string normalized_default = current_default;
      // std::replace(normalized_default.begin(), normalized_default.end(), ' ', '_');
      // std::transform(normalized_default.begin(), normalized_default.end(), normalized_default.begin(), ::tolower);
      std::string current_default = std::get<std::string>(entry.default_value);
      std::string normalized_default = config_utils::NormalizeString(current_default);

      bool found = false;
      for (const auto& name : profile_names) {
        // std::string norm_name = name;
        std::string norm_name = config_utils::NormalizeString(name);
        // std::replace(norm_name.begin(), norm_name.end(), ' ', '_');
        // std::transform(norm_name.begin(), norm_name.end(), norm_name.begin(), ::tolower);

        if (norm_name == normalized_default) {
          found = true;
          break;
        }
      }

      if (!found && !profile_names.empty()) {
        // std::string first = profile_names[0];
        // std::replace(first.begin(), first.end(), ' ', '_');
        // std::transform(first.begin(), first.end(), first.begin(), ::tolower);
        std::string first = config_utils::NormalizeString(profile_names[0]);
        entry.default_value = first;
      }

      FLOG_DEBUG("Updated profile config: %s", enum_str.c_str());
      break;
    }
  }
}

// void Heater::Tune() {
//   if (!_target.has_value()) {
//     // AssertOff();
//     // FLOG_ERROR("No target temperature set, skipping PID");
//     return;
//   }
//   /*
//   uint32_t _time_slice;           // time slice in ms
//   uint32_t _temperature_delta;    // proportional
//   uint32_t _rate_of_change;       // current temp - prev / over time (differential)
//   uint32_t _temperature_integral; // integral - > (_temperature_delta * _time_slice) + _temperature_integral
//   uint32_t _power_setting;        // Power or PWM setting
//   uint32_t _kp;                   // 0.01? // get in range
//   uint32_t _kd = 0;               // tweak for overshoot
//   uint32_t _ki = 0;               // tweak for steady state error*/
//   /*
//     _temperature_delta = _target - _temperature;
//   FLOG_DEBUG("Temp: %d, Target: %d, Delta: %d", _temperature, _target, _temperature_delta);

//   _temperature_integral = (_temperature_delta * _time_slice) + _temperature_integral;
//   _rate_of_change = _temperature - _previous_temperature / _time_slice;
//   FLOG_DEBUG("D: %d ROC: %d I: %d", _temperature_delta, _rate_of_change, _temperature_integral);
//   _power_setting = (_kp * _temperature_delta) + (_kd * _rate_of_change) + (_ki * _temperature_integral);
//   _previous_temperature = _temperature;
//   */

//   FLOG_TRACE("==== PID =========================================");
//   _temperature_delta = _target.value() - _temperature;
//   FLOG_TRACE("Temp: %d, Target: %d, Delta: %d", _temperature, _target.value(), _temperature_delta);

//   // Integral term with windup protection
//   _temperature_integral = (_temperature_delta * _time_slice) + _temperature_integral;

//   // Clamp integral to prevent windup
//   const int max_integral = 50000;  // Adjust based on your system
//   if (_temperature_integral > max_integral) _temperature_integral = max_integral;
//   if (_temperature_integral < -max_integral) _temperature_integral = -max_integral;

//   // Derivative term with zero division protection
//   // _rate_of_change = (_temperature - _previous_temperature) / _time_slice;
//   _rate_of_change = (_time_slice > 0) ? (float)(_temperature - _previous_temperature) / _time_slice : 0;
//   FLOG_TRACE("I: %.02f,  RoC: (%d - %d) / %d -> %.02f", _temperature_integral, _temperature, _previous_temperature,
//              _time_slice, _rate_of_change);

//   _power_setting = (_kp * _temperature_delta) + (_kd * _rate_of_change) + (_ki * _temperature_integral);

//   // Clamp output to valid range (0-100% or 0-255, etc.)
//   if (_power_setting < 0) _power_setting = 0;
//   if (_power_setting > 100) _power_setting = 100;  // Adjust max value for your system

//   FLOG_TRACE("Pwr setting: %.02f", _power_setting);
//   // printf(">delta:%d, rate:%.02f, integral:%.02f, power:%.02f\r\n", _temperature_delta, _rate_of_change,
//   //        _temperature_integral, _power_setting);

//   _previous_temperature = _temperature;
// }

void Heater::Tune() {
  if (_state != heater::kStateOn) return;
  if (!_target.has_value()) return;

  // Convert to °C for intuitive PID gains
  float error_degC = (_target.value() - _temperature) / 100.0f;
  float rate_degC_per_sec = ((_temperature - _previous_temperature) / 100.0f) / (_time_slice / 1000.0f);

  // Integral in °C×seconds
  _temperature_integral += error_degC * (_time_slice / 1000.0f);

  // Anti-windup (now in °C×sec, ~500 = 10°C for 50 sec)
  constexpr float kMaxIntegral = 500.0f;
  _temperature_integral = std::clamp(_temperature_integral, -kMaxIntegral, kMaxIntegral);

  // PID output: gains now mean "% power per unit"
  // Kp=5 means 5% power per 1°C error
  // Ki=0.1 means 0.1% power per °C×sec
  // Kd=1 means 1% power per °C/sec
  _power_setting = (_kp * error_degC) - (_kd * rate_degC_per_sec) + (_ki * _temperature_integral);

  _power_setting = std::clamp(_power_setting, 0.0f, 100.0f);
  _previous_temperature = _temperature;

  FLOG_TRACE("PID: err=%.1f°C rate=%.2f°C/s I=%.1f pwr=%.1f%%", error_degC, rate_degC_per_sec, _temperature_integral,
             _power_setting);
}

esp_err_t Heater::StateToOn() {
  _stall_counter = 0;
  if (_state == heater::kStateOff) {
    _start_time_ms = esp_timer_get_time() / 1000;
  }
  // TODO: centralize safety checks before allowing ON state
  if (!_element) {
    FLOG_ERROR("No heater element configured, cannot turn on");
    PS_PUB_ERR_FL(kTopicStatusError, ESP_ERR_INVALID_STATE, "No heater element configured, cannot turn on",
                  PS_FL_STICKY);
    StateToOff();
    return ESP_ERR_INVALID_STATE;
  }

  switch (_mode) {
    case heater::Mode::kModeDrying:
      if (_time_remaining_ms <= 0 && _timer_ms != 0) {
        _time_remaining_ms = _timer_ms;
      }
      break;
    case heater::Mode::kModeReflow:
      if (!_current_profile) {
        FLOG_ERROR("No profile loaded, cannot start profile mode");
        // PS_PUB_ERR_FL(topics::heater::error, ESP_ERR_INVALID_STATE, "No profile loaded, cannot start profile mode",
        //               PS_FL_STICKY);
        PS_PUB_ERR(kTopicStatusWarning, ESP_ERR_INVALID_STATE, "No profile loaded, cannot start profile mode");
        return ESP_ERR_INVALID_STATE;
      }
      _start_time_ms = esp_timer_get_time() / 1000;
      break;
    default:
      return ESP_ERR_NOT_SUPPORTED;
      break;
  }
  _state = heater::kStateOn;
  FLOG_INFO("Heater changed state to ON in mode %s", ToString(_mode).c_str());
  PS_PUB_INT_FL(topics::heater::state, _state, PS_FL_STICKY);
  return ESP_OK;
}

esp_err_t Heater::StateToOff() {
  HeaterOff();
  switch (_mode) {
    case heater::Mode::kModeDrying:
      ClearTimer();
      ClearTarget();
      _state = heater::kStateOff;
      // ClearTarget();
      break;
    case heater::Mode::kModeReflow:
      ClearTarget();
      _start_time_ms = 0;
      PS_PUB_NIL(topics::heater::profile_stage);
      _state = heater::kStateOff;
      break;
    default:
      ClearTarget();
      _state = heater::kStateOff;
      PS_PUB_INT_FL(topics::heater::state, _state, PS_FL_STICKY);
      return ESP_ERR_NOT_SUPPORTED;
      break;
  }
  FLOG_INFO("Heater changed state to OFF");
  PS_PUB_INT_FL(topics::heater::state, _state, PS_FL_STICKY);
  return ESP_OK;
}

esp_err_t Heater::StateToPause() {
  HeaterOff();
  switch (_mode) {
    case heater::Mode::kModeDrying:
      _state = heater::kStatePause;
      break;
    case heater::Mode::kModeReflow:
      return ESP_ERR_NOT_SUPPORTED;
      break;
    default:
      return ESP_ERR_NOT_SUPPORTED;
      break;
  }
  FLOG_INFO("Heater changed state to PAUSE");
  PS_PUB_INT_FL(topics::heater::state, _state, PS_FL_STICKY);
  return ESP_OK;
}

esp_err_t Heater::UpdateTimer() {
  // FLOG_INFO("UpdateTimer called");
  static int64_t last = esp_timer_get_time();
  if (_timer_ms == 0 && _time_remaining_ms != 0) {
    _time_remaining_ms = 0;
    PS_PUB_NIL_FL(topics::heater::timer_remaining, PS_FL_STICKY);
    FLOG_ERROR("Timer cleared externally");
    return ESP_OK;
  }
  if (_time_remaining_ms <= 0) {
    return ESP_OK;
  }

  switch (_state) {
    case heater::kStateOn:
      _time_remaining_ms -= (esp_timer_get_time() - last) / 1000;
      break;
    default:
      break;
  }
  last = esp_timer_get_time();
  FLOG_DEBUG("timer remaining: %lli", _time_remaining_ms / 1000);
  return ESP_OK;
}

esp_err_t Heater::LoopProfile() {
  // TODO: Optimize
  if (_current_profile && _state == heater::kStateOn) {
    uint32_t elapsed = (esp_timer_get_time() / 1000) - _start_time_ms;
    if (elapsed > _current_profile->TotalDuration()) {
      SetState(heater::kStateOff);
      ClearTarget();
      FLOG_INFO("Heater profile complete");
      return ESP_OK;
      ;
    }
    int32_t profile_target = static_cast<int32_t>(_current_profile->TargetTemp(elapsed) * 100.0f);
    if (!_target.has_value() || profile_target != _target.value()) {
      SetTarget(profile_target);
      FLOG_DEBUG("Profile target temperature updated to %li", profile_target);
    }
    std::string stage = _current_profile->CurrentStage(elapsed);
    PS_PUB_STR_FL(topics::heater::profile_stage, stage.c_str(), PS_FL_STICKY);
  }
  return ESP_OK;
}

esp_err_t Heater::LoopDryer() {
  static int64_t refresh = esp_timer_get_time();
  if (_time_remaining_ms <= 0 && _state == heater::kStateOn) {
    SetState(heater::kStateOff);
    _time_remaining_ms = 0;
    FLOG_INFO("Dryer cycle complete");
    if (_timer_ms > 0) PS_PUB_INT_FL(topics::heater::timer_remaining, _timer_ms / 100, PS_FL_STICKY);
    return ESP_OK;
  }
  if (refresh + 1000 * 1000 < esp_timer_get_time()) {
    if (_time_remaining_ms > 0) {
      refresh = esp_timer_get_time();
      // _time_remaining_ms -= 1000;
      FLOG_DEBUG("Timer remaining: %lli seconds", _time_remaining_ms / 1000);
      PS_PUB_INT_FL(topics::heater::timer_remaining, _time_remaining_ms / 1000, PS_FL_STICKY);
      // } else {
      //   if (_timer_ms > 0) {
      //     PS_PUB_INT_FL(topics::heater::timer_remaining, _timer_ms / 1000, PS_FL_STICKY);
      //   } else {
      //     PS_PUB_NIL_FL("topics::heater::timer_remaining, PS_FL_STICKY);
      //   }
    }
  }
  return ESP_OK;
}

}  // namespace toothless