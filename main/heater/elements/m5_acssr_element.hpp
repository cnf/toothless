#pragma once
#include <driver/gpio.h>
#include <esp_err.h>

#include <cstdint>

#include "config.h"
#include "heater/elements/element.hpp"
#include "i2c_manager.hpp"

namespace toothless {
static constexpr uint16_t kACSSRDefaultAddress = 0x50;
static constexpr uint16_t kACSSRRelayRegister = 0x00;
static constexpr uint16_t kACSSRLEDRegister = 0x10;
static constexpr uint16_t kACSSRAddressRegister = 0x20;
static constexpr uint16_t kACSSRVersionRegister = 0xFE;

class M5I2CElement : public BaseElement {
 public:
  esp_err_t Init() override;
  bool IsOn() const override;

 private:
  i2c_master_dev_handle_t _dev_handle;
  std::shared_ptr<I2cManager> _i2c_mgr;
  esp_err_t PowerOn(uint8_t duty);
  esp_err_t PowerOff();
  esp_err_t Power(bool on);
  uint8_t Version();
  esp_err_t SetLEDColor(uint32_t colorHEX);
};

}  // namespace toothless