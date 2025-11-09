#include "heater/heater.hpp"

#include <esp_err.h>
#include <esp_timer.h>

#include "funlog.h"
#include "heater.hpp"
#include "heater/elements/element.hpp"
#include "heater/elements/gpio_element.hpp"

namespace toothless {
Profile::Stage lead_free_stages[] = {
    {25, 150, 90000, Profile::Shape::Smooth, "Preheat"},
    {150, 180, 90000, Profile::Shape::Linear, "Soak"},
    {180, 240, 30000, Profile::Shape::Smooth, "Ramp to Peak"},
    {240, 100, 120000, Profile::Shape::Smooth, "Cooldown"},
};

Profile lead_free(lead_free_stages, 4);

Profile::Stage leaded_stages[] = {
    {25, 90, 90000, Profile::Shape::Smooth, "Preheat"},     //
    {90, 130, 90000, Profile::Shape::Linear, "Soak"},       //
    {130, 138, 45000, Profile::Shape::Smooth, "Ramp Up"},   //
    {138, 165, 30000, Profile::Shape::Smooth, "Reflow"},    //
    {165, 100, 30000, Profile::Shape::Linear, "Cooldown"},  //
};
Profile leaded(leaded_stages, 5);

using namespace heater;

bool Heater::Init() {
  _element = std::make_unique<GPIOElement>();
  _element->Init();
  _time_slice = 200;
  _kp = 0.01;  // expected 0.01 - 2.0
  _kd = 0;     // expected 0.0 - 50
  _ki = 0;     // expected 0.0001 - 0.01
  _target = std::numeric_limits<int32_t>::quiet_NaN();
  _temperature_integral = 0;
  _previous_temperature = std::numeric_limits<int32_t>::max();
  _last_run = esp_timer_get_time() * 1000;
  LoadProfile("Qwik Leaded");
  _mode = heater::kModeDrying;  // TODO: configure

  _subscription = ps_new_subscriber(10, PS_STRLIST("sensor.temperature", "heater"));
  return true;
}

void Heater::Loop() {
  HandleSubscriptions();
  // auto temperature = GetTemperature();
  if (_last_temp_update + 5000000 < esp_timer_get_time()) {
    AssertOff();
    if (_state != heater::kStateOff) {
      SetState(heater::kStateOff);
      PS_PUB_STR("heater.status", "error: temperature sensor timeout");
      FLOG_ERROR("No temperature updates received for heater, turning off");
    };
    return;
  }
  if (_state != heater::kStateOn) {
    AssertOff();
    return;
  }
  // TODO: Optimize
  if (_current_profile && _state == heater::kStateOn) {
    uint32_t elapsed = (esp_timer_get_time() - _start_time) / 1000;
    if (elapsed > _current_profile->TotalDuration()) {
      SetState(heater::kStateIdle);
      ClearTarget();
      FLOG_INFO("Heater profile complete");
      return;
    }
    int32_t profile_target = static_cast<int32_t>(_current_profile->TargetTemp(elapsed) * 100.0f);
    if (profile_target != _target) {
      SetTarget(profile_target);
      FLOG_DEBUG("Profile target temperature updated to %d", profile_target);
    }
    std::string stage = _current_profile->CurrentStage(elapsed);
    PS_PUB_STR("heater.profile.stage", stage.c_str());
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
  // uint32_t tdelta = esp_timer_get_time() - _last_run;
  ps_msg_t* msg = NULL;
  // uint32_t temperature;
  for ((msg = ps_get(_subscription, 0)); msg != NULL; (msg = ps_get(_subscription, 0))) {
    if (ps_has_topic(msg, "sensor.temperature.chamber") && PS_IS_INT(msg)) {
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
        SetState(heater::kStateIdle);
        FLOG_ERROR("Invalid heater state message");
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
        PS_PUB_STR_FL("heater.profile", "Qwik Leaded", PS_FL_STICKY);  // TODO: profile name
      } else {
        PS_PUB_NIL_FL("heater.profile", PS_FL_STICKY);
      }
    } else {
      FLOG_DEBUG("unknown message : %s", msg->topic);
    }
    ps_unref_msg(msg);
  }
  return ESP_OK;
}

void Heater::AssertOff() { HeaterOff(); }

esp_err_t Heater::LoadProfile(std::string name) {
  _current_profile = &leaded;
  // PS_PUB_STR_FL("heater.profile", _current_profile, PS_FL_STICKY);
  PS_PUB_STR_FL("heater.profile", name.c_str(), PS_FL_STICKY);
  return ESP_OK;
}

esp_err_t Heater::StartProfile() {
  //
  return ESP_OK;
}

std::optional<uint16_t> Heater::GetTemperature() { return std::nullopt; }

esp_err_t Heater::SetState(heater::State state) {
  switch (state) {
    case heater::kStateOn:
      _state = heater::kStateOn;
      _start_time = esp_timer_get_time();
      FLOG_DEBUG("Heater state: ON");
      break;
    case heater::kStateOff:
      _state = heater::kStateOff;
      ClearTarget();
      _start_time = 0;
      PS_PUB_NIL("heater.profile.stage");
      FLOG_DEBUG("Heater state: OFF");
      break;
    case heater::kStatePause:
      _state = heater::kStatePause;
      FLOG_DEBUG("Heater state: PAUSE");
      break;
    case heater::kStateIdle:
      _state = heater::kStateIdle;
      FLOG_DEBUG("Heater state: IDLE");

      break;
    default:
      FLOG_ERROR("Unknown heater state %d", state);
      return ESP_ERR_INVALID_ARG;
  }
  PS_PUB_INT_FL("heater.state", _state, PS_FL_STICKY);
  return ESP_OK;
}

esp_err_t Heater::SetMode(heater::Mode mode) {
  switch (_mode) {
    case heater::kModeHeating:
      _mode = heater::kModeHeating;
      PS_PUB_INT_FL("heater.mode", _mode, PS_FL_STICKY);
      FLOG_DEBUG("Heater mode: HEATING");
      break;
    case heater::kModeDrying:
      _mode = heater::kModeDrying;
      PS_PUB_INT_FL("heater.mode", _mode, PS_FL_STICKY);
      FLOG_DEBUG("Heater mode: DRYING");
      break;
    case heater::kModeProfile:
      _mode = heater::kModeProfile;
      PS_PUB_INT_FL("heater.mode", _mode, PS_FL_STICKY);
      FLOG_DEBUG("Heater mode: PROFILE");
      break;
    case heater::kModeCooldown:
      _mode = heater::kModeCooldown;
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

}  // namespace toothless