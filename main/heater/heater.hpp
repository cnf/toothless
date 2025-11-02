#pragma once

#include <esp_err.h>
#include <limits>
#include <optional>
#include <stdint.h>

extern "C" {
#include <pubsub.h>
}

namespace toothless {
class Heater {
public:
  Heater(int8_t pin);
  bool Init();
  void Loop();
  std::optional<uint16_t> GetTemperature();
  esp_err_t SetPower(uint8_t power);
  esp_err_t HeaterOn();
  esp_err_t HeaterOff();

private:
  ps_subscriber_t *_sub_temp;
  int8_t _pin;
  uint32_t _target;
  uint32_t _last_temp_update;
  uint32_t _last_run;
  int32_t _temperature;
  int32_t _previous_temperature = std::numeric_limits<uint32_t>::max();
  // PID
  // power setting is _kp * _temperature_delta  + _kd * _rate_of_change + _ki * _temperature_integral
  uint32_t _time_slice;        // time slice in ms
  int32_t _temperature_delta;  // proportional
  float _rate_of_change;       // current temp - prev / over time (differential)
  float _temperature_integral; // integral - > (_temperature_delta * _time_slice) + _temperature_integral
  float _power_setting;        // Power or PWM setting
  float _kp;                   // 0.01? // get in range
  float _kd = 0;               // tweak for overshoot
  float _ki = 0;               // tweak for steady state error

  void Tune();
};
} // namespace toothless