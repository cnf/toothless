#pragma once

#include <functional>
#include <memory>
#include <vector>

#include "config_mgr.hpp"
#include "peripheral.hpp"

namespace toothless {

namespace topics::peripherals::sensors {
inline constexpr char const* const name = "sensor";
inline constexpr char const* const temperature = "sensor.temperature";
inline constexpr char const* const humidity = "sensor.humidity";
inline constexpr char const* const current = "sensor.current";

}  // namespace topics::peripherals::sensors

struct PeripheralConfig {
  std::string mode;
  std::string profile;
  uint16_t max_temp;
  float pid_kp;
  float pid_kd;
  float pid_ki;
};

inline ConfigEntries config_entries = {
    ConfigEntry("zone_temp", "Zone Temperature Sensor", "enum=", std::string(""), ""),
    ConfigEntry("zone_heater", "Heater Element", "enum=", std::string(""), ""),
};

class PeripheralRegistry {
 public:
  using ProbeFunc = std::function<bool()>;
  using FactoryFunc = std::function<std::shared_ptr<Peripheral>()>;

  struct Registration {
    PeripheralInfo info;
    ProbeFunc probe;
    FactoryFunc create;
  };

  static void Init();
  static void Enable(const char* name);
  static void Disable(const char* name);
  static void Loop();  // Runs all enabled peripherals

  static void Register(Registration reg);
  static std::vector<PeripheralInfo> ProbeAll();
  static void RegisterZoneConfig();
  static std::shared_ptr<Peripheral> Create(const char* name);
  static const std::vector<PeripheralInfo> GetEnabledInfo();
  static std::vector<PeripheralInfo> GetDetectedByType(const char* type);
  static std::string GetPeripheralTopic(const char* name);

 private:
  static std::vector<Registration>& GetRegistry();
  static std::vector<std::shared_ptr<Peripheral>>& GetEnabled();
};
}  // namespace toothless