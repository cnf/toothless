#pragma once

#include <esp_err.h>
#include <stdint.h>

#include <limits>
#include <memory>
#include <optional>

#include "config_mgr.hpp"
#include "heater/profiles/profile.hpp"
#include "heater/profiles/profile_manager.hpp"
#include "peripherals/actuators/actuator.hpp"
#include "topics.hpp"

extern "C" {
#include <pubsub.h>
}

namespace toothless {

namespace topics::heater {
static constexpr const char name[10] = "heater";
static constexpr const char start[13] = "heater.start";
static constexpr const char stop[12] = "heater.stop";
static constexpr const char pause[13] = "heater.pause";
static constexpr const char power[16] = "heater.power";

static constexpr const char target[24] = "heater.target";
static constexpr const char target_temperature[36] = "heater.target.temperature";
static constexpr const char target_temperature_set[40] = "heater.target.temperature.set";

static constexpr const char state[16] = "heater.state";
static constexpr const char state_set[20] = "heater.state.set";

static constexpr const char mode[14] = "heater.mode";
static constexpr const char mode_set[18] = "heater.mode.set";

static constexpr const char profile[18] = "heater.profile";
static constexpr const char profile_set[22] = "heater.profile.set";
static constexpr const char profile_get[22] = "heater.profile.get";
static constexpr const char profile_changed[26] = "heater.profile.changed";

static constexpr const char profile_stage[24] = "heater.profile.stage";

static constexpr const char timer[20] = "heater.timer";
static constexpr const char timer_set[24] = "heater.timer.set";
static constexpr const char timer_remaining[26] = "heater.timer.remaining";
inline const char* GetErrorTopic() {
  static std::string s = std::string(kTopicStatusError) + TOPIC_DOT + name;
  return s.c_str();
}
static const char* error = GetErrorTopic();

}  // namespace topics::heater

namespace heater {

enum class Mode { kModeHeating, kModeCooldown, kModeReflow, kModeDrying };
enum State { kStateOff, kStateOn, kStatePause };

static constexpr std::pair<Mode, const char*> kModeMap[] = {
    {Mode::kModeReflow, "reflow"},
    {Mode::kModeDrying, "drying"},
    {Mode::kModeHeating, "heating"},
    {Mode::kModeCooldown, "cooldown"},
};

inline std::string ToString(Mode t) {
  for (auto& [mode, name] : kModeMap)
    if (mode == t) return name;
  return "unknown";
}

inline Mode FromString(const std::string& name) {
  auto normalize = [](const std::string& s) {
    std::string result;
    for (char c : s) {
      if (c == ' ' || c == '-')
        result += '_';
      else
        result += std::toupper(c);
    }
    return result;
  };

  std::string normalized = normalize(name);
  // std::string normalized =

  for (const auto& [id, mode_name] : kModeMap) {
    if (normalized == normalize(mode_name)) {
      return id;
    }
  }

  return Mode::kModeReflow;
}

inline std::string MakeFormat() {
  std::string f = "enum=";
  for (size_t i = 0; i < std::size(kModeMap); ++i) {
    if (i > 0) f += "|";
    f += kModeMap[i].second;
  }
  return f;
}

struct SafetyLimits {
  float min_heat_rate = 0.5f;         // °C/sec minimum when power > 50%
  float max_heat_rate = 10.0f;        // °C/sec maximum (sensor sanity)
  int32_t overshoot_limit = 1500;     // 15°C above target = emergency
  uint32_t stall_timeout_ms = 30000;  // 30s no progress = abort
  uint32_t sensor_timeout_ms = 5000;  // 5s no temp updates = abort
};

struct HeaterConfig {
  std::string mode;
  std::string profile;
  uint16_t max_temp;
  float pid_kp;
  float pid_kd;
  float pid_ki;
};

inline ConfigEntries config_entries = {
    ConfigEntry("mode", "Default Mode", MakeFormat(), std::string("reflow"), ""),
    // This will be updated dynamically in Init()
    ConfigEntry("profile", "Default Heater Profile", "enum=", std::string("chip_quik_leaded"), ""),
    ConfigEntry("max_temp", "Maximum target temperature the heater will accept", "min=50,max=500", 250, "°C"),
    ConfigEntry("pid_kp", "[P] Power % per °C error", "min=0.1,max=50,step=0.1", 5.0f, "%/°C"),
    ConfigEntry("pid_ki", "[I] Power % per °C*sec", "min=0.001,max=2,step=0.01", 0.1f, "%/°C*s"),
    ConfigEntry("pid_kd", "[D] Power % per °C/sec rate", "min=0,max=20,step=0.1", 1.0f, "%*s/°C"),
    // ConfigEntry("pid_kp", "Heater PID Proportional constant", "min=0.01,max=2.0,step=0.01", 0.01f, "Kp"),
    // ConfigEntry("pid_ki", "Heater PID Integral constant", "min=0.0001 ,max=0.01,step=0.0001", 0.0f, "Ki"),
    // ConfigEntry("pid_kd", "Heater PID Differential constant", "min=0.0,max=50,step=0.1", 0.0f, "Kd"),

};

}  // namespace heater
class Heater {
 public:
  Heater();
  ~Heater();
  bool Init();
  void Loop();
  esp_err_t HandleSubscriptions();
  esp_err_t ApplySettings();
  esp_err_t GetElement();
  /// @brief Assert heater is off
  void AssertOff();
  /// @brief Check for heating anomalies
  /// @return ESP_OK if safe, error code if shutdown needed
  esp_err_t CheckSafety();
  esp_err_t LoadProfile(std::string name);
  /// @brief Refresh profile list in config manager
  esp_err_t RefreshProfileConfig();

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
  esp_err_t HeaterOn(uint8_t power);
  esp_err_t HeaterOff();

 private:
  ps_subscriber_t* _subscription;
  std::unique_ptr<ConfigEntries> _config_entries;  // UI configuration entries
  std::shared_ptr<SettingsMap> _config;            // UI settings map
  // std::unique_ptr<BaseElement> _element;
  std::shared_ptr<Actuator> _element;
  heater::State _state = heater::State::kStateOff;
  heater::Mode _mode = heater::Mode::kModeReflow;
  std::unique_ptr<heater::SafetyLimits> _limits;
  uint32_t _stall_counter = 0;
  // Profiles
  std::shared_ptr<ProfileManager> _profile_mgr;
  std::shared_ptr<Profile> _current_profile;  // Use shared_ptr
  int32_t _maximum_temperature;

  int64_t _start_time_ms;  // time we started the current profile
  int64_t _timer_ms = 0;
  int64_t _time_remaining_ms = 0;
  std::optional<int32_t> _target = std::nullopt;  // target temperature
  uint32_t _last_temp_update = 0;                 // temp sensor failsafe
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

  /// @brief Update profile config entry with current profiles
  void UpdateProfileConfigEntry();

  void CalibratePID();
  void Tune();
  esp_err_t StateToOn();
  esp_err_t StateToOff();
  esp_err_t StateToPause();
  esp_err_t UpdateTimer();
  esp_err_t LoopProfile();
  esp_err_t LoopDryer();
};  // namespace topicsclass Heater
}  // namespace toothless