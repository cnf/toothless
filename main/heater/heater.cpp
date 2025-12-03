#include "heater/heater.hpp"

#include <esp_err.h>
#include <esp_timer.h>

#include "funlog.h"
#include "heater.hpp"
#include "heater/elements/element.hpp"
#include "heater/elements/gpio_element.hpp"
#include "heater/elements/m5_acssr_element.hpp"
#include "heater/profiles/profile_manager.hpp"

namespace toothless {

using namespace heater;

bool Heater::Init() {
  // esp_log_level_set(FLOG_SHORT_FILENAME, ESP_LOG_DEBUG);

  // Set up Config
  _config = std::make_shared<SettingsMap>();
  _config_entries = new ConfigEntries;
  _config_entries->insert(std::end(*_config_entries), std::begin(config_entries), std::end(config_entries));

  // Set up Profile manager
  _profile_mgr = ProfileManager::GetInstance();  // std::make_shared<ProfileManager>();
  // _profile_mgr->Init();
  _profile_mgr->CreateDefaults();
  UpdateProfileConfigEntry();

  // Register Config
  RegisterConfig(_config_entries, topics::heater::name);
  FLOG_DEBUG("Waiting for settings...");
  GetSettings(_config, topics::heater::name);

  _element = std::make_unique<GPIOElement>();
  // _element = std::make_unique<M5I2CElement>();
  esp_err_t err = _element->Init();
  if (err != ESP_OK) {
    FLOG_ERROR("Heater element initialization failed");
    _element = std::make_unique<M5I2CElement>();
  }
  _time_slice = 200;
  _kp = 0.01;  // expected 0.01 - 2.0
  _kd = 0;     // expected 0.0 - 50
  _ki = 0;     // expected 0.0001 - 0.01
  _target = std::numeric_limits<int32_t>::quiet_NaN();
  _temperature_integral = 0;
  _previous_temperature = std::numeric_limits<int32_t>::max();
  _last_run = esp_timer_get_time() * 1000;
  // SetMode(heater::Mode::kModeDrying);  // TODO: configure

  SetMode(FromString(std::get<std::string>(_config->at("mode"))));
  LoadProfile(std::get<std::string>(_config->at("profile")));

  _subscription = ps_new_subscriber(10, PS_STRLIST("sensor.temperature", "heater", "profiles.changed"));
  return true;
}

void Heater::Loop() {
  HandleSubscriptions();
  UpdateTimer();

  if (_last_temp_update + 5000000 < esp_timer_get_time()) {
    AssertOff();
    if (_state != heater::kStateOff) {
      SetState(heater::kStateOff);
      PS_PUB_STR("heater.status", "error: temperature sensor timeout");
      FLOG_ERROR("No temperature updates received for heater, turning off");
    };
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

  if (_state != heater::kStateOn) {
    AssertOff();
    return;
  }

  Tune();
  // FLOG_INFO("T: %0.2fC POWER: %d", _temperature / 100.0f, _power_setting);
  if (_power_setting > 0) {
    HeaterOn(_power_setting);
  } else {
    HeaterOff();
  }

  _last_run = esp_timer_get_time() * 1000;
}

esp_err_t Heater::HandleSubscriptions() {
  bool new_settings = false;
  // uint32_t tdelta = esp_timer_get_time() - _last_run;
  ps_msg_t* msg = NULL;
  // uint32_t temperature;
  for ((msg = ps_get(_subscription, 0)); msg != NULL; (msg = ps_get(_subscription, 0))) {
    if (ps_has_topic(msg, "sensor.temperature.zone") && PS_IS_INT(msg)) {
      _temperature = msg->int_val;
      if (_previous_temperature == std::numeric_limits<int32_t>::max()) {
        _previous_temperature = _temperature;
      }
      _last_temp_update = esp_timer_get_time();
    } else if (ps_has_topic(msg, "heater.state.set")) {
      if (PS_IS_INT(msg)) {
        heater::State state = static_cast<heater::State>(msg->int_val);
        SetState(state);
      } else {
        SetState(heater::kStateOff);
        FLOG_ERROR("Invalid heater state message");
      }
    } else if (ps_has_topic(msg, "heater.timer.set")) {
      if (PS_IS_INT(msg)) {
        SetTimer(msg->int_val);
      } else if (PS_IS_NIL(msg)) {
        ClearTimer();
      } else {
        FLOG_ERROR("Invalid heater timer message");
      }
    } else if (ps_has_topic(msg, "heater.mode.set") && PS_IS_INT(msg)) {
      heater::Mode mode = static_cast<heater::Mode>(msg->int_val);
      SetMode(mode);
    } else if (ps_has_topic(msg, "heater.start")) {
      SetState(heater::kStateOn);
      // HeaterOn();
      FLOG_INFO("Heater started");
    } else if (ps_has_topic(msg, "heater.stop")) {
      SetState(heater::kStateOff);
      // HeaterOff();
      FLOG_INFO("Heater stopped");
    } else if (ps_has_topic(msg, "heater.pause")) {
      SetState(heater::kStatePause);
      FLOG_INFO("Heater paused");
    } else if (ps_has_topic(msg, "heater.target.temperature.set")) {
      if (PS_IS_INT(msg)) {
        SetTarget(msg->int_val);
        FLOG_INFO("Heater target set to %d", _target);
      } else if (PS_IS_NIL(msg)) {
        ClearTarget();
        FLOG_INFO("Heater target cleared");
      } else {
        FLOG_ERROR("Invalid heater target temperature message");
      }
    } else if (ps_has_topic(msg, "heater.profile.get")) {
      if (_current_profile) {
        PS_PUB_STR_FL("heater.profile", _current_profile->Name().c_str(), PS_FL_STICKY);  // TODO: profile name
      } else {
        PS_PUB_NIL_FL("heater.profile", PS_FL_STICKY);
      }
    } else if (ps_has_topic(msg, "heater.profile.set")) {
      if (PS_IS_STR(msg)) {
        LoadProfile(std::string(msg->str_val));
      } else {
        FLOG_ERROR("Invalid heater profile message");
      }
    } else if (ps_has_topic(msg, "profiles.changed")) {
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

void Heater::AssertOff() { HeaterOff(); }

// esp_err_t Heater::LoadProfile(std::string name) {
//   _current_profile = &leaded;
//   // PS_PUB_STR_FL("heater.profile", _current_profile, PS_FL_STICKY);
//   PS_PUB_STR_FL("heater.profile", name.c_str(), PS_FL_STICKY);
//   return ESP_OK;
// }

esp_err_t Heater::LoadProfile(std::string name) {
  _current_profile = _profile_mgr->GetProfile(name);
  if (!_current_profile) {
    FLOG_ERROR("Profile '%s' not found", name.c_str());
    return ESP_ERR_NOT_FOUND;
  }
  PS_PUB_STR_FL("heater.profile", name.c_str(), PS_FL_STICKY);
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
      PS_PUB_INT_FL("heater.mode", _mode, PS_FL_STICKY);
      FLOG_DEBUG("Heater mode: HEATING");
      break;
    case heater::Mode::kModeDrying:
      _mode = heater::Mode::kModeDrying;
      PS_PUB_INT_FL("heater.mode", _mode, PS_FL_STICKY);
      FLOG_DEBUG("Heater mode: DRYING");
      break;
    case heater::Mode::kModeReflow:
      _mode = heater::Mode::kModeReflow;
      PS_PUB_INT_FL("heater.mode", _mode, PS_FL_STICKY);
      FLOG_DEBUG("Heater mode: REFLOW");
      break;
    case heater::Mode::kModeCooldown:
      _mode = heater::Mode::kModeCooldown;
      PS_PUB_INT_FL("heater.mode", _mode, PS_FL_STICKY);
      FLOG_DEBUG("Heater mode: COOLDOWN");
      break;
    default:
      FLOG_ERROR("Unknown heater mode %d", _mode);
      return ESP_ERR_INVALID_ARG;
  }
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
  PS_PUB_INT_FL("heater.timer", _timer_ms / 1000, PS_FL_STICKY);
  PS_PUB_INT_FL("heater.timer.remaining", _timer_ms / 1000, PS_FL_STICKY);
  FLOG_INFO("Heater timer set to %d seconds", time_sec);
  return ESP_OK;
}

esp_err_t Heater::ClearTimer() {
  _timer_ms = 0;
  PS_PUB_NIL_FL("heater.timer", PS_FL_STICKY);
  PS_PUB_NIL_FL("heater.timer.remaining", PS_FL_STICKY);

  return ESP_OK;
}

esp_err_t Heater::SetTarget(int32_t target) {
  if (target > 99900 || target < -9900) {
    FLOG_ERROR("Target temperature %d out of range (-99 to 999)", target);
    return ESP_ERR_INVALID_ARG;
  }
  _target = target;
  PS_PUB_INT_FL("heater.target.temperature", _target, PS_FL_STICKY);
  return ESP_OK;
}

esp_err_t Heater::ClearTarget() {
  _target = std::numeric_limits<int32_t>::quiet_NaN();
  PS_PUB_NIL_FL("heater.target.temperature", PS_FL_STICKY);
  FLOG_DEBUG("Cleared target temperature");
  return ESP_OK;
};

esp_err_t Heater::HeaterOn(float power) {
  // TODO: figure out what unit power is in
  return _element->On((uint8_t)power * 100);
}

esp_err_t Heater::HeaterOff() { return _element->Off(); }

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
        std::string normalized = profile_names[i];
        std::replace(normalized.begin(), normalized.end(), ' ', '_');
        std::transform(normalized.begin(), normalized.end(), normalized.begin(), ::tolower);
        enum_str += normalized;
      }

      entry.format = enum_str;

      // Update default value to normalized first profile if current isn't valid
      std::string current_default = std::get<std::string>(entry.default_value);
      std::string normalized_default = current_default;
      std::replace(normalized_default.begin(), normalized_default.end(), ' ', '_');
      std::transform(normalized_default.begin(), normalized_default.end(), normalized_default.begin(), ::tolower);

      bool found = false;
      for (const auto& name : profile_names) {
        std::string norm_name = name;
        std::replace(norm_name.begin(), norm_name.end(), ' ', '_');
        std::transform(norm_name.begin(), norm_name.end(), norm_name.begin(), ::tolower);

        if (norm_name == normalized_default) {
          found = true;
          break;
        }
      }

      if (!found && !profile_names.empty()) {
        std::string first = profile_names[0];
        std::replace(first.begin(), first.end(), ' ', '_');
        std::transform(first.begin(), first.end(), first.begin(), ::tolower);
        entry.default_value = first;
      }

      FLOG_DEBUG("Updated profile config: %s", enum_str.c_str());
      break;
    }
  }
}

void Heater::Tune() {
  /*
  uint32_t _time_slice;           // time slice in ms
  uint32_t _temperature_delta;    // proportional
  uint32_t _rate_of_change;       // current temp - prev / over time (differential)
  uint32_t _temperature_integral; // integral - > (_temperature_delta * _time_slice) + _temperature_integral
  uint32_t _power_setting;        // Power or PWM setting
  uint32_t _kp;                   // 0.01? // get in range
  uint32_t _kd = 0;               // tweak for overshoot
  uint32_t _ki = 0;               // tweak for steady state error*/
  /*
    _temperature_delta = _target - _temperature;
  FLOG_DEBUG("Temp: %d, Target: %d, Delta: %d", _temperature, _target, _temperature_delta);

  _temperature_integral = (_temperature_delta * _time_slice) + _temperature_integral;
  _rate_of_change = _temperature - _previous_temperature / _time_slice;
  FLOG_DEBUG("D: %d ROC: %d I: %d", _temperature_delta, _rate_of_change, _temperature_integral);
  _power_setting = (_kp * _temperature_delta) + (_kd * _rate_of_change) + (_ki * _temperature_integral);
  _previous_temperature = _temperature;
  */

  FLOG_DEBUG("==== PID =========================================");
  _temperature_delta = _target - _temperature;
  FLOG_DEBUG("Temp: %d, Target: %d, Delta: %d", _temperature, _target, _temperature_delta);

  // Integral term with windup protection
  _temperature_integral = (_temperature_delta * _time_slice) + _temperature_integral;

  // Clamp integral to prevent windup
  const int max_integral = 50000;  // Adjust based on your system
  if (_temperature_integral > max_integral) _temperature_integral = max_integral;
  if (_temperature_integral < -max_integral) _temperature_integral = -max_integral;

  // Derivative term with zero division protection
  // _rate_of_change = (_temperature - _previous_temperature) / _time_slice;
  _rate_of_change = (_time_slice > 0) ? (float)(_temperature - _previous_temperature) / _time_slice : 0;
  FLOG_DEBUG("I: %.02f,  RoC: (%d - %d) / %d -> %.02f", _temperature_integral, _temperature, _previous_temperature,
             _time_slice, _rate_of_change);

  _power_setting = (_kp * _temperature_delta) + (_kd * _rate_of_change) + (_ki * _temperature_integral);

  // Clamp output to valid range (0-100% or 0-255, etc.)
  if (_power_setting < 0) _power_setting = 0;
  if (_power_setting > 100) _power_setting = 100;  // Adjust max value for your system

  FLOG_DEBUG("Pwr setting: %.02f", _power_setting);
  // printf(">delta:%d, rate:%.02f, integral:%.02f, power:%.02f\r\n", _temperature_delta, _rate_of_change,
  //        _temperature_integral, _power_setting);

  _previous_temperature = _temperature;
}

esp_err_t Heater::StateToOn() {
  if (_state == heater::kStateOff) {
    _start_time_ms = esp_timer_get_time() / 1000;
  }

  switch (_mode) {
    case heater::Mode::kModeDrying:
      if (_time_remaining_ms == 0 && _timer_ms != 0) {
        _time_remaining_ms = _timer_ms;
      }
      break;
    case heater::Mode::kModeReflow:
      if (!_current_profile) {
        FLOG_ERROR("No profile loaded, cannot start profile mode");
        return ESP_ERR_INVALID_STATE;
      }
      _start_time_ms = esp_timer_get_time() / 1000;
      break;
    default:
      return ESP_ERR_NOT_SUPPORTED;
      break;
  }
  _state = heater::kStateOn;
  FLOG_DEBUG("Heater state: ON");
  PS_PUB_INT_FL("heater.state", _state, PS_FL_STICKY);
  return ESP_OK;
}

esp_err_t Heater::StateToOff() {
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
      PS_PUB_NIL("heater.profile.stage");
      _state = heater::kStateOff;
      break;
    default:
      ClearTarget();
      _state = heater::kStateOff;
      PS_PUB_INT_FL("heater.state", _state, PS_FL_STICKY);
      return ESP_ERR_NOT_SUPPORTED;
      break;
  }
  FLOG_DEBUG("Heater state: OFF");
  PS_PUB_INT_FL("heater.state", _state, PS_FL_STICKY);
  return ESP_OK;
}

esp_err_t Heater::StateToPause() {
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
  FLOG_DEBUG("Heater state: PAUSE");
  PS_PUB_INT_FL("heater.state", _state, PS_FL_STICKY);
  return ESP_OK;
}

esp_err_t Heater::UpdateTimer() {
  // FLOG_INFO("UpdateTimer called");
  static int64_t last = esp_timer_get_time();
  if (_timer_ms == 0 && _time_remaining_ms != 0) {
    _time_remaining_ms = 0;
    PS_PUB_NIL_FL("heater.timer.remaining", PS_FL_STICKY);
    return ESP_OK;
  }
  switch (_state) {
    case heater::kStateOn:
      if (_time_remaining_ms <= 0) break;
      _time_remaining_ms -= (esp_timer_get_time() - last) / 1000;
      break;
    default:
      break;
  }
  last = esp_timer_get_time();

  // }
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
    if (profile_target != _target) {
      SetTarget(profile_target);
      FLOG_DEBUG("Profile target temperature updated to %d", profile_target);
    }
    std::string stage = _current_profile->CurrentStage(elapsed);
    PS_PUB_STR("heater.profile.stage", stage.c_str());
  }
  return ESP_OK;
}

esp_err_t Heater::LoopDryer() {
  static int64_t refresh = esp_timer_get_time();
  if (_time_remaining_ms <= 0 && _state == heater::kStateOn) {
    SetState(heater::kStateOff);
    _time_remaining_ms = 0;
    FLOG_INFO("Dryer cycle complete");
    if (_timer_ms > 0) PS_PUB_INT_FL("heater.timer.remaining", _timer_ms / 100, PS_FL_STICKY);
    return ESP_OK;
  }
  if (refresh + 1000000 < esp_timer_get_time()) {
    if (_time_remaining_ms > 0) {
      refresh = esp_timer_get_time();
      // _time_remaining_ms -= 1000;
      FLOG_DEBUG("Timer remaining: %d seconds", _time_remaining_ms / 1000);
      PS_PUB_INT_FL("heater.timer.remaining", _time_remaining_ms / 1000, PS_FL_STICKY);
      // } else {
      //   if (_timer_ms > 0) {
      //     PS_PUB_INT_FL("heater.timer.remaining", _timer_ms / 1000, PS_FL_STICKY);
      //   } else {
      //     PS_PUB_NIL_FL("heater.timer.remaining", PS_FL_STICKY);
      //   }
    }
  }
  return ESP_OK;
}

}  // namespace toothless