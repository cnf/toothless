#include "heater/heater.hpp"
#include "funlog.h"
#include "heater.hpp"
#include <driver/gpio.h>
#include <esp_err.h>
#include <esp_timer.h>

namespace toothless {
Heater::Heater(int8_t pin) { _pin = pin; }

bool Heater::Init() {
  _time_slice = 200;
  _kp = 0.01; // expected 0.01 - 2.0
  _kd = 0;    // expected 0.0 - 50
  _ki = 0;    // expected 0.0001 - 0.01
  _target = 3000;
  _temperature_integral = 0;
  _previous_temperature = std::numeric_limits<int32_t>::max();
  _last_run = esp_timer_get_time() * 1000;

  ESP_ERROR_CHECK(gpio_set_direction((gpio_num_t)_pin, GPIO_MODE_OUTPUT));
  ESP_ERROR_CHECK(gpio_set_level((gpio_num_t)_pin, 0));
  _sub_temp = ps_new_subscriber(1, PS_STRLIST("sensor.chamber.temperature"));
  return true;
}

void Heater::Loop() {
  uint32_t tdelta = esp_timer_get_time() - _last_run;
  ps_msg_t *msg = NULL;
  // uint32_t temperature;
  msg = ps_get(_sub_temp, 0);
  if (msg != NULL) {
    if (PS_IS_INT(msg)) {
      _temperature = msg->int_val;
      if (_previous_temperature == std::numeric_limits<int32_t>::max()) {
        _previous_temperature = _temperature;
      }
      _last_temp_update = esp_timer_get_time();
    }
  }
  // auto temperature = GetTemperature();
  if (_last_temp_update + 5000000 < esp_timer_get_time()) {
    HeaterOff();
    PS_PUB_STR("heater.status", "error: no temperature data");
    return;
  }
  Tune();
  // FLOG_INFO("T: %0.2fC POWER: %d", _temperature / 100.0f, _power_setting);

  // // TODO: PID etc...
  // if (_target > temperature) {
  //   HeaterOn();
  // } else {
  //   HeaterOff();
  // }
  _last_run = esp_timer_get_time() * 1000;
}

std::optional<uint16_t> Heater::GetTemperature() { return std::nullopt; }

esp_err_t Heater::SetPower(uint8_t power) {
  if (power > 0) {
    HeaterOn();
  } else {
    HeaterOff();
  }
}

esp_err_t Heater::HeaterOn() {
  PS_PUB_BOOL("heater.power", true);
  return gpio_set_level((gpio_num_t)_pin, 1);
}

esp_err_t Heater::HeaterOff() {
  PS_PUB_BOOL("heater.power", false);
  return gpio_set_level((gpio_num_t)_pin, 0);
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
  const int max_integral = 50000; // Adjust based on your system
  if (_temperature_integral > max_integral)
    _temperature_integral = max_integral;
  if (_temperature_integral < -max_integral)
    _temperature_integral = -max_integral;

  // Derivative term with zero division protection
  // _rate_of_change = (_temperature - _previous_temperature) / _time_slice;
  _rate_of_change = (_time_slice > 0) ? (float)(_temperature - _previous_temperature) / _time_slice : 0;
  FLOG_DEBUG("I: %.02f,  RoC: (%d - %d) / %d -> %.02f", _temperature_integral, _temperature, _previous_temperature,
             _time_slice, _rate_of_change);

  _power_setting = (_kp * _temperature_delta) + (_kd * _rate_of_change) + (_ki * _temperature_integral);

  // Clamp output to valid range (0-100% or 0-255, etc.)
  if (_power_setting < 0)
    _power_setting = 0;
  if (_power_setting > 100)
    _power_setting = 100; // Adjust max value for your system

  FLOG_DEBUG("Pwr setting: %.02f", _power_setting);
  // printf(">delta:%d, rate:%.02f, integral:%.02f, power:%.02f\r\n", _temperature_delta, _rate_of_change,
  //        _temperature_integral, _power_setting);

  _previous_temperature = _temperature;
}

} // namespace toothless