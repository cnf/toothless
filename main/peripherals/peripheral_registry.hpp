#pragma once

#include <driver/gpio.h>

#include <expected>
#include <functional>
#include <memory>
#include <vector>

#include "actuators/actuator.hpp"
#include "config_mgr.hpp"
#include "esp_err.h"
#include "peripheral.hpp"
#include "sensors/sensor.hpp"

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

inline ConfigEntries perf_config_entries = {
    ConfigEntry("zone_temp", "Zone Temperature Sensor", "enum=", std::string(""), ""),
    ConfigEntry("zone_heater", "Zone Heater Element", "enum=", std::string(""), ""),
    ConfigEntry("probe", "Probe Sensor", "enum=", std::string(""), ""),
    ConfigEntry("fan", "Fan Element", "enum=", std::string(""), ""),
};

class PeripheralRegistry {
 public:
  using ProbeFunc = std::function<bool()>;
  using FactoryFunc = std::function<std::shared_ptr<Peripheral>()>;
  struct PinSlot {
    const char* key;          // "ctrl_pin"
    std::string description;  // "SSR Control Pin"
  };
  struct Registration {
    PeripheralInfo info;
    ProbeFunc probe;
    FactoryFunc create;
    enum class Type { kSensor, kActuator } type;
    std::vector<PinSlot> pin_slots;  // Empty for non-GPIO peripherals
  };
  static PeripheralRegistry& Instance() {
    static PeripheralRegistry instance;
    return instance;
  }

  static void Register(Registration reg) { Instance().GetRegistry().push_back(reg); }

  void Init();
  void Enable(const char* name);
  void Disable(const char* name);
  void Loop();

  // esp_err_t ApplySettings(std::shared_ptr<SettingsMap> config);

  // static void Register(Registration reg);
  std::vector<PeripheralInfo> ProbeAll();
  void RegisterZoneConfig();
  std::shared_ptr<Peripheral> Create(const char* name);
  const std::vector<PeripheralInfo> GetEnabledInfo();
  std::vector<PeripheralInfo> GetDetectedByType(std::variant<SensorType, ActuatorType> type);
  std::vector<PeripheralInfo> GetDetectedByType(const char* type);
  std::string GetPeripheralTopic(const char* name);

  std::shared_ptr<Sensor> GetSensor(std::string name);
  std::shared_ptr<Actuator> GetActuator(std::string name);
  const std::vector<std::shared_ptr<Sensor>>& GetEnabledSensors() const;
  const std::vector<std::shared_ptr<Actuator>>& GetEnabledActuators() const;

  static std::string MakeGpioFormat();
  static std::expected<gpio_num_t, esp_err_t> GpioFromString(const std::string& pin_str);

  // void RegisterGPIOConfigs();

 private:
  ps_subscriber_t* _subscriptions = nullptr;
  std::shared_ptr<SettingsMap> _config;
  std::unique_ptr<ConfigEntries> _config_entries;
  std::shared_ptr<Sensor> _zone_temp_sensor = nullptr;
  std::shared_ptr<Actuator> _zone_heater = nullptr;
  std::shared_ptr<Sensor> _probe_sensor = nullptr;
  std::shared_ptr<Actuator> _fan_actuator = nullptr;

  std::vector<std::shared_ptr<Sensor>> _enabled_sensors;
  std::vector<std::shared_ptr<Actuator>> _enabled_actuators;

  PeripheralRegistry();   // = default;  // Private constructor
  ~PeripheralRegistry();  // = default;
  PeripheralRegistry(const PeripheralRegistry&) = delete;
  PeripheralRegistry& operator=(const PeripheralRegistry&) = delete;

  std::vector<Registration>& GetRegistry() {
    static std::vector<Registration> registry;
    return registry;
  }
  static std::vector<std::shared_ptr<Peripheral>>& GetEnabled();
  void HandleSubscriptions();
  esp_err_t ApplySettings(std::shared_ptr<SettingsMap> config);
};
}  // namespace toothless