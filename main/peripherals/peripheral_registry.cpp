#include "peripheral_registry.hpp"

#include <esp_err.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include <algorithm>
#include <cstring>

#include "funlog.h"
// #include <

extern "C" {
#include <pubsub.h>
}

namespace toothless {

void PeripheralRegistry::Init() {
  for (auto& peripheral : GetRegistry()) {
    FLOG_INFO("Registered peripheral: %s", peripheral.info.name);
  }
  ProbeAll();
  RegisterZoneConfig();
}

void PeripheralRegistry::Enable(const char* name) {
  auto peripheral = Create(name);
  if (peripheral && peripheral->Init() == ESP_OK) {
    GetEnabled().push_back(peripheral);
    FLOG_INFO("Enabled peripheral: %s", name);
    // PS_PUB_BOOL("peripheral.enabled", true);  // pubsub notify
  }
}

void PeripheralRegistry::Disable(const char* name) {
  auto& enabled = GetEnabled();
  std::erase_if(enabled, [name](const auto& p) { return strcmp(p->Info().name, name) == 0; });
}

void PeripheralRegistry::Loop() {
  for (auto& peripheral : GetEnabled()) {
    peripheral->Loop();
    vTaskDelay(5 / portTICK_PERIOD_MS);
  }
}
void PeripheralRegistry::Register(Registration reg) { GetRegistry().push_back(reg); }

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
  config_entries[0].format = temp_enum;

  auto heaters = PeripheralRegistry::GetDetectedByType("ssr");
  std::string heater_enum = "enum=";
  for (const auto& p : heaters) {
    if (heater_enum.length() > 5) heater_enum += "|";
    // heater_enum += p.name;
    heater_enum += config_utils::NormalizeString(p.name);
  }
  config_entries[1].format = heater_enum;

  ConfigManager::GetInstance()->RegisterSettings("peripheral", &config_entries);
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

std::vector<PeripheralInfo> PeripheralRegistry::GetDetectedByType(const char* type) {
  std::vector<PeripheralInfo> result;
  for (const auto& reg : GetRegistry()) {
    if (reg.probe() && strcmp(reg.info.type, type) == 0) {
      result.push_back(reg.info);
    }
  }
  return result;
}

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

std::vector<PeripheralRegistry::Registration>& PeripheralRegistry::GetRegistry() {
  static std::vector<Registration> registry;
  return registry;
}
std::vector<std::shared_ptr<Peripheral>>& PeripheralRegistry::GetEnabled() {
  static std::vector<std::shared_ptr<Peripheral>> enabled;
  return enabled;
}
}  // namespace toothless