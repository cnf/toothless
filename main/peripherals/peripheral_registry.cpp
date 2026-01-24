#include "peripheral_registry.hpp"

#include <esp_err.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include <algorithm>
#include <cstring>

#include "funlog.h"
#include "helpers/box_mover.hpp"
#include "helpers/string_to_snake.hpp"
#include "implementation.hpp"
#include "peripherals/sensors/sensor.hpp"
#include "sdkconfig.h"
// #include "heater/heater.hpp"

// #include <esp_heap_caps.h>

extern "C" {
#include <pubsub.h>
}

namespace toothless {

// TODO: implement reprobing / hotplug detection

PeripheralRegistry::PeripheralRegistry() {
  _config = std::make_shared<SettingsMap>();
  _config_entries = std::make_unique<ConfigEntries>();
  _subscriptions = ps_new_subscriber(10, PS_STRLIST("peripheral"));
};

PeripheralRegistry::~PeripheralRegistry() {
  if (_subscriptions) {
    ps_free_subscriber(_subscriptions);
    _subscriptions = nullptr;
  }
  FLOG_ERROR("PeripheralRegistry destroyed");
};

void PeripheralRegistry::Init() {
  _config_entries->insert(std::end(*_config_entries), std::begin(perf_config_entries), std::end(perf_config_entries));
  for (auto& peripheral : GetRegistry()) {
    FLOG_INFO("Registered peripheral: %s", peripheral.info.name);
  }
  ProbeAll();
  RegisterZoneConfig();
  GetSettings(_config, "peripheral");
  ApplySettings(_config);
}

// void PeripheralRegistry::Enable(const char* name) {
//   auto peripheral = Create(name);
//   if (peripheral && peripheral->Init() == ESP_OK) {
//     GetEnabled().push_back(peripheral);
//     FLOG_INFO("Enabled peripheral: %s", name);
//     // PS_PUB_BOOL("peripheral.enabled", true);  // pubsub notify
//   }
// }

void PeripheralRegistry::Enable(const char* name) {
  auto peripheral = Create(name);
  if (peripheral && peripheral->Init() == ESP_OK) {
    GetEnabled().push_back(peripheral);

    // Find registration to get type
    for (const auto& reg : GetRegistry()) {
      if (strcmp(reg.info.name, name) == 0) {
        if (reg.type == Registration::Type::kSensor) {
          _enabled_sensors.push_back(std::static_pointer_cast<Sensor>(peripheral));
        } else {
          _enabled_actuators.push_back(std::static_pointer_cast<Actuator>(peripheral));
        }
        break;
      }
    }
    FLOG_INFO("[x] %s enabled", name);
  }
}

void PeripheralRegistry::Disable(const char* name) {
  auto& enabled = GetEnabled();
  auto it =
      std::find_if(enabled.begin(), enabled.end(), [name](const auto& p) { return strcmp(p->Info().name, name) == 0; });
  if (it != enabled.end()) {
    // Find and remove from type-specific collections
    std::erase_if(_enabled_sensors, [name](const auto& s) { return strcmp(s->Info().name, name) == 0; });
    std::erase_if(_enabled_actuators, [name](const auto& a) { return strcmp(a->Info().name, name) == 0; });
    enabled.erase(it);
  }
}

// void PeripheralRegistry::Disable(const char* name) {
//   auto& enabled = GetEnabled();
//   std::erase_if(enabled, [name](const auto& p) { return strcmp(p->Info().name, name) == 0; });
// }

// void PeripheralRegistry::Loop() {
//   HandleSubscriptions();
//   for (auto& peripheral : GetEnabled()) {
//     peripheral->Loop();
//     vTaskDelay(5 / portTICK_PERIOD_MS);
//   }
// }

void PeripheralRegistry::Loop() {
  HandleSubscriptions();
  auto enabled_copy =
      GetEnabled();  // copy shared_ptr list, so that if one is disabled during Loop, we don't invalidate the iterator
  for (auto& peripheral : enabled_copy) {
    peripheral->Loop();
    vTaskDelay(5 / portTICK_PERIOD_MS);
  }
}

void PeripheralRegistry::HandleSubscriptions() {
  ps_msg_t* msg = NULL;
  for ((msg = ps_get(_subscriptions, 0)); msg != NULL; (msg = ps_get(_subscriptions, 0))) {
    // if (!heap_caps_check_integrity(MALLOC_CAP_DEFAULT, true)) {
    //   FLOG_ERROR("Heap bad before sub loop");
    //   abort();
    // }
    if (ps_has_topic_prefix(msg, "peripheral.config.get")) {
      FLOG_INFO("Peripheral config get message received");
      GetSettings(_config, "peripheral");
      ApplySettings(_config);
      ps_unref_msg(msg);
      // if (!heap_caps_check_integrity(MALLOC_CAP_DEFAULT, true)) {
      //   FLOG_ERROR("Heap bad after proc msg %s", msg->topic);
      //   abort();
      // }
      continue;
    } else if (ps_has_topic(msg, "peripheral.actuator.zone.get") && PS_IS_PTR(msg)) {
      FLOG_INFO("PeripheralRegistry: actuator.zone.get message received");
      if (_zone_heater) {
        FLOG_INFO("Getting heater");
        auto* sp_ptr = static_cast<std::shared_ptr<Actuator>*>(msg->ptr_val);
        *sp_ptr = _zone_heater;  // Dereference and assign
        // auto ptr = helper::Unbox<Actuator>(msg->ptr_val);
        // ptr = _zone_heater;
        if (msg->rtopic) {
          PS_PUB_NIL(msg->rtopic);
        }
      }
      // if (!heap_caps_check_integrity(MALLOC_CAP_DEFAULT, true)) {
      //   FLOG_ERROR("Heap bad after proc msg %s", msg->topic);
      //   abort();
      // }
    } else {
      // FLOG_WARN("Unknown topic prefix for topic %s", msg->topic);
    }
    ps_unref_msg(msg);
  }
  return;
}

esp_err_t PeripheralRegistry::ApplySettings(std::shared_ptr<SettingsMap> config) {
  std::string zone_temp = std::get<std::string>((*config)["zone_temp"]);
  std::string zone_heater = std::get<std::string>((*config)["zone_heater"]);
  std::string probe = std::get<std::string>((*config)["probe"]);
  std::string fan = std::get<std::string>((*config)["fan"]);

  if (_zone_temp_sensor) _zone_temp_sensor->ClearAltTopic();
  _zone_temp_sensor = GetSensor(zone_temp);
  if (_zone_temp_sensor) _zone_temp_sensor->SetAltTopic("sensor.temperature.zone");

  if (_probe_sensor) _probe_sensor->ClearAltTopic();
  _probe_sensor = GetSensor(probe);
  if (_probe_sensor) _probe_sensor->SetAltTopic("sensor.temperature.probe");

  _fan_actuator = GetActuator(std::get<std::string>((*config)["fan"]));
  PS_PUB_NIL("peripheral.fan.config.applied");
  _zone_heater = GetActuator(zone_heater);
  PS_PUB_NIL("peripheral.zone_heater.config.applied");

  return ESP_OK;
}

std::vector<PeripheralInfo> PeripheralRegistry::ProbeAll() {
  std::vector<PeripheralInfo> detected;
  for (const auto& reg : GetRegistry()) {
    if (reg.probe()) {
      detected.push_back(reg.info);
      Enable(reg.info.name);
    }
  }
  return detected;
}

void PeripheralRegistry::RegisterZoneConfig() {
  // Build enum string from detected peripherals
  auto temps = PeripheralRegistry::GetDetectedByType("temperature");
  std::string temp_enum = "enum=";
  for (const auto& p : temps) {
    if (temp_enum.length() > 5) temp_enum += "|";
    // temp_enum += p.name;
    temp_enum += config_utils::NormalizeString(p.name);
  }
  perf_config_entries[0].format = temp_enum;

  std::string probe_enum = "enum=";
  for (const auto& p : temps) {
    if (probe_enum.length() > 5) probe_enum += "|";
    // probe_enum += p.name;
    probe_enum += config_utils::NormalizeString(p.name);
  }
  probe_enum += "|none";
  perf_config_entries[2].format = probe_enum;

  auto heaters = PeripheralRegistry::GetDetectedByType("ssr");
  std::string heater_enum = "enum=";
  for (const auto& p : heaters) {
    if (heater_enum.length() > 5) heater_enum += "|";
    // heater_enum += p.name;
    heater_enum += config_utils::NormalizeString(p.name);
  }
  perf_config_entries[1].format = heater_enum;

  RegisterConfig(&perf_config_entries, "peripheral");
}

std::shared_ptr<Peripheral> PeripheralRegistry::Create(const char* name) {
  for (const auto& reg : GetRegistry()) {
    if (strcmp(reg.info.name, name) == 0) {
      return reg.create();
    }
  }
  return nullptr;
}

const std::vector<PeripheralInfo> PeripheralRegistry::GetEnabledInfo() {
  std::vector<PeripheralInfo> infos;
  for (const auto& p : GetEnabled()) {
    infos.push_back(p->Info());
  }
  return infos;
}

std::vector<PeripheralInfo> PeripheralRegistry::GetDetectedByType(std::variant<SensorType, ActuatorType> type) {
  std::vector<PeripheralInfo> result;
  for (const auto& en : GetEnabled()) {
    if (std::holds_alternative<SensorType>(type) && std::holds_alternative<SensorType>(en->Info().type)) {
      SensorType sensor_type = std::get<SensorType>(en->Info().type);
      if (sensor_type == std::get<SensorType>(type)) result.push_back(en->Info());

    } else if (std::holds_alternative<ActuatorType>(type) && std::holds_alternative<ActuatorType>(en->Info().type)) {
      ActuatorType actuator_type = std::get<ActuatorType>(en->Info().type);
      if (actuator_type == std::get<ActuatorType>(type)) result.push_back(en->Info());
    }
  };
  // for (const auto& reg : GetRegistry()) {
  //   // if (!reg.probe()) continue;
  //   if (std::holds_alternative<SensorType>(type) && std::holds_alternative<SensorType>(reg.info.type)) {
  //     SensorType sensor_type = std::get<SensorType>(reg.info.type);
  //     if (sensor_type == std::get<SensorType>(type)) result.push_back(reg.info);

  //   } else if (std::holds_alternative<ActuatorType>(type) && std::holds_alternative<ActuatorType>(reg.info.type)) {
  //     ActuatorType actuator_type = std::get<ActuatorType>(reg.info.type);
  //     if (actuator_type == std::get<ActuatorType>(type)) result.push_back(reg.info);
  //   }
  // }
  return result;
}

std::vector<PeripheralInfo> PeripheralRegistry::GetDetectedByType(const char* type) {
  std::vector<PeripheralInfo> result;
  for (const auto& en : GetEnabled()) {
    if (std::holds_alternative<SensorType>(en->Info().type)) {
      SensorType sensor_type = std::get<SensorType>(en->Info().type);
      if (SensorTypeToString(sensor_type) == type) result.push_back(en->Info());

    } else if (std::holds_alternative<ActuatorType>(en->Info().type)) {
      ActuatorType actuator_type = std::get<ActuatorType>(en->Info().type);
      if (ActuatorTypeToString(actuator_type) == type) result.push_back(en->Info());
    }
  };
  // for (const auto& reg : GetRegistry()) {
  //   // if (!reg.probe()) continue;
  //   if (std::holds_alternative<SensorType>(reg.info.type)) {
  //     SensorType sensor_type = std::get<SensorType>(reg.info.type);
  //     if (SensorTypeToString(sensor_type) == type) result.push_back(reg.info);

  //   } else if (std::holds_alternative<ActuatorType>(reg.info.type)) {
  //     ActuatorType actuator_type = std::get<ActuatorType>(reg.info.type);
  //     if (ActuatorTypeToString(actuator_type) == type) result.push_back(reg.info);
  //   }
  // }
  return result;
}

// std::vector<PeripheralInfo> PeripheralRegistry::GetDetectedByType(const char* type) {
//   std::vector<PeripheralInfo> result;
//   for (const auto& reg : GetRegistry()) {
//     if (reg.probe() && strcmp(reg.info.type, type) == 0) {
//       result.push_back(reg.info);
//     }
//   }
//   return result;
// }

std::string PeripheralRegistry::GetPeripheralTopic(const char* name) {
  std::string normalized = config_utils::NormalizeString(name);
  for (const auto& p : GetEnabled()) {
    if (config_utils::NormalizeString(p->Info().name).compare(normalized) == 0) {
      return p->Topic();
    }
    if (strcmp(p->Info().name, name) == 0) {
      return p->Topic();
    }
  }
  return std::string();
}

// std::vector<PeripheralRegistry::Registration>& PeripheralRegistry::GetRegistry() {
//   static std::vector<Registration> registry;
//   return registry;
// }
std::vector<std::shared_ptr<Peripheral>>& PeripheralRegistry::GetEnabled() {
  static std::vector<std::shared_ptr<Peripheral>> enabled;
  return enabled;
}

std::shared_ptr<Sensor> PeripheralRegistry::GetSensor(std::string name) {
  for (const auto& sensor : _enabled_sensors) {
    if (config_utils::NormalizeString(sensor->Info().name).compare(config_utils::NormalizeString(name)) == 0) {
      return sensor;
    }
  }
  return nullptr;
}

std::shared_ptr<Actuator> PeripheralRegistry::GetActuator(std::string name) {
  for (const auto& actuator : _enabled_actuators) {
    if (config_utils::NormalizeString(actuator->Info().name).compare(config_utils::NormalizeString(name)) == 0) {
      return actuator;
    }
  }
  return nullptr;
}

const std::vector<std::shared_ptr<Sensor>>& PeripheralRegistry::GetEnabledSensors() const { return _enabled_sensors; }

const std::vector<std::shared_ptr<Actuator>>& PeripheralRegistry::GetEnabledActuators() const {
  return _enabled_actuators;
}

// void PeripheralRegistry::RegisterGPIOConfigs() {
//   for (auto& reg : GetRegistry()) {
//     if (reg.pin_slots.empty()) continue;

//     ConfigEntries entries;
//     std::string pin_enum = MakeGpioFormat();

//     for (auto& slot : reg.pin_slots) {
//       entries.push_back(ConfigEntry(slot.key, slot.description, pin_enum, std::string("None"), std::string("io")));
//     }

//     RegisterConfig(&entries, StringToSnake(reg.info.name).c_str());
//   }
// }

std::string PeripheralRegistry::MakeGpioFormat() {
  std::string result = "enum=None|";
  for (size_t i = 0; i < std::size(impl::kGpioFreeList); i++) {
    if (i > 0) result += "|";
    result += "gpio_" + std::to_string(impl::kGpioFreeList[i]);
  }
  return result;
}

std::expected<gpio_num_t, esp_err_t> PeripheralRegistry::GpioFromString(const std::string& pin_str) {
  if (pin_str == "None") {
    return GPIO_NUM_NC;
  }
  if (pin_str.rfind("gpio_", 0) == 0) {
    // no exception handeling is enabled
    int pin_num = std::stoi(pin_str.substr(5));
    for (const auto& gpio : impl::kGpioFreeList) {
      if (static_cast<int>(gpio) == pin_num) {
        return gpio;
      }
    }
  }
  return GPIO_NUM_NC;  // Default to not connected
}

}  // namespace toothless