#pragma once
#include <driver/gpio.h>
#include <esp_err.h>

#include <cstdint>
#include <memory>

#include "config.h"
#include "config_mgr.hpp"
#include "peripherals/actuators/actuator.hpp"

namespace toothless {

namespace topics::peripherals::gpio_ssr {
inline constexpr const char* const name = "gpio_ssr";
inline constexpr const char* const gpio_ssr = "actuator.gpio_ssr";
}  // namespace topics::peripherals::gpio_ssr

inline constexpr const char* kGPIOElementName = "GPIO SSR";
inline constexpr BusType kGPIOElementBusType = BusType::kGPIO;
inline constexpr uint8_t kGPIOElementAddress = 0x00;

struct GPIOEConfig {
  std::string ctrl_pin = "None";
  bool src_sink = false;  // true = source (T), false = sink (F)
};

inline ConfigEntries gpio_config_entries = {
    // ConfigEntry("ctrl_pin", "Element Control pin", "enum=", std::string("None"), "GPIO"),
    ConfigEntry("ctrl_pin", "Element Control pin", PeripheralRegistry::BuildGpioPinEnum(), std::string("None"), "io"),
    ConfigEntry("src_sink", "Source [T] or Sink [F]", "", false, ""),
};

class GPIOElement : public Actuator {
 public:
  static bool Detect() { return true; }  // Always available
  esp_err_t Init() override;
  esp_err_t Loop() override;
  const PeripheralInfo& Info() const override;
  bool IsOn() const override;
  static const PeripheralInfo& GetInfo() { return _info; }
  static std::shared_ptr<GPIOElement> GetInstance() {
    static auto instance = std::make_shared<GPIOElement>();
    return instance;
  }

 private:
  std::shared_ptr<SettingsMap> _config;
  std::unique_ptr<ConfigEntries> _config_entries;
  static const PeripheralInfo _info;
  gpio_num_t _pin;
  esp_err_t PowerOn(uint8_t duty);
  esp_err_t PowerOff();
};
}  // namespace toothless