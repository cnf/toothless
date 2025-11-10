#pragma once

#include <esp_err.h>
#include <stdint.h>

#include <limits>
#include <memory>
#include <optional>

#include "heater/elements/element.hpp"
#include "heater/profile.hpp"
#include "topics/topics.hpp"

extern "C" {
#include <pubsub.h>
}

namespace toothless {
// namespace topics::heater {
// // inline constexpr auto base_arr = topic::concat("heater", "");

// inline constexpr auto base = topic::concat("heater", "");
// inline constexpr auto power = topic::concat(base, "power");
// // inline constexpr char* power = "heater.power";
// inline constexpr auto target = topic::concat(base, "target");
// inline constexpr auto target_temperature = topic::concat(target, "temperature");
// //"heater.target.temperature";
// }  // namespace topics::heater

namespace heater {

enum Mode { kModeHeating, kModeCooldown, kModeReflow, kModeDrying };
enum State { kStateOff, kStateOn, kStatePause };

}  // namespace heater
class Heater {
 public:
  Heater() {};
  bool Init();
  void Loop();
  esp_err_t HandleSubscriptions();
  void AssertOff();
  esp_err_t LoadProfile(std::string name);
  std::optional<uint16_t> GetTemperature();
  uint64_t GetTimeSecondsLeft();

  esp_err_t SetState(heater::State state);
  esp_err_t SetMode(heater::Mode mode);
  esp_err_t SetPower(uint8_t power);
  esp_err_t SetTimer(uint32_t time_sec);
  esp_err_t ClearTimer();
  esp_err_t SetTarget(int32_t target);
  esp_err_t ClearTarget();

  esp_err_t HeaterOn(float power);
  esp_err_t HeaterOff();

 private:
  ps_subscriber_t* _subscription;
  std::unique_ptr<BaseElement> _element;
  heater::State _state;
  heater::Mode _mode;
  // std::unique_ptr<Profile> _current_profile;
  Profile* _current_profile = nullptr;
  int64_t _start_time_ms;  // time we started the current profile
  int64_t _timer_ms = 0;
  int64_t _time_remaining_ms = 0;
  int32_t _target = std::numeric_limits<int32_t>::quiet_NaN();
  uint32_t _last_temp_update;  // temp sensor failsafe
  uint32_t _last_run;
  int32_t _temperature;
  int32_t _previous_temperature = std::numeric_limits<uint32_t>::max();
  // PID
  // power setting is _kp * _temperature_delta  + _kd * _rate_of_change + _ki * _temperature_integral
  uint32_t _time_slice;         // time slice in ms
  int32_t _temperature_delta;   // proportional
  float _rate_of_change;        // current temp - prev / over time (differential)
  float _temperature_integral;  // integral - > (_temperature_delta * _time_slice) + _temperature_integral
  float _power_setting;         // Power or PWM setting
  float _kp;                    // 0.01? // get in range
  float _kd = 0;                // tweak for overshoot
  float _ki = 0;                // tweak for steady state error

  void Tune();
  esp_err_t StateToOn();
  esp_err_t StateToOff();
  esp_err_t StateToPause();
  esp_err_t UpdateTimer();
  esp_err_t LoopProfile();
  esp_err_t LoopDryer();
};  // namespace topicsclass Heater
}  // namespace toothless