#pragma once
#include <driver/gpio.h>
#include <esp_err.h>

#include <cstdint>

#include "config.h"
#include "i2c_manager.hpp"
#include "peripherals/actuators/actuator.hpp"

namespace toothless {
inline constexpr const char* kACSSRName = "M5 ACSSR";
inline constexpr BusType kACSSRBusType = BusType::kI2C;

static constexpr uint16_t kACSSRDefaultAddress = 0x50;
static constexpr uint16_t kACSSRRelayRegister = 0x00;
static constexpr uint16_t kACSSRLEDRegister = 0x10;
static constexpr uint16_t kACSSRAddressRegister = 0x20;
static constexpr uint16_t kACSSRVersionRegister = 0xFE;

class M5I2CElement : public Actuator {
 public:
  esp_err_t Init() override;
  bool IsOn() const override;
  esp_err_t Loop() override;
  const PeripheralInfo& Info() const override;
  static const PeripheralInfo& GetInfo() { return _info; }
  static bool Detect();
  static std::shared_ptr<M5I2CElement> GetInstance() {
    static auto instance = std::make_shared<M5I2CElement>();
    return instance;
  }

 private:
  static const PeripheralInfo _info;
  i2c_master_dev_handle_t _dev_handle;
  bool _is_on = false;
  std::shared_ptr<I2cManager> _i2c_mgr;
  esp_err_t PowerOn(uint8_t duty) override;
  esp_err_t PowerOff();
  esp_err_t Power(bool on);
  uint8_t Version();
  esp_err_t SetLEDColor(uint32_t colorHEX);
};

}  // namespace toothless