#pragma once
#include <driver/gpio.h>
#include <esp_err.h>

#include <cstdint>

#include "config.h"
#include "heater/elements/element.hpp"
#include "i2c_manager.hpp"

namespace toothless {

class M5I2CElement : public BaseElement {
 public:
  esp_err_t Init() override;
  bool IsOn() const override;

 private:
  i2c_master_dev_handle_t _dev_handle;
  std::shared_ptr<I2cManager> _i2c_mgr;
  esp_err_t PowerOn(uint8_t duty);
  esp_err_t PowerOff();
};
}  // namespace toothless