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

std::vector<PeripheralRegistry::Registration>& PeripheralRegistry::GetRegistry() {
  static std::vector<Registration> registry;
  return registry;
}
std::vector<std::shared_ptr<Peripheral>>& PeripheralRegistry::GetEnabled() {
  static std::vector<std::shared_ptr<Peripheral>> enabled;
  return enabled;
}
}  // namespace toothless