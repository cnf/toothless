#pragma once
#include <driver/gpio.h>
#include <esp_err.h>

#include <cstdint>

#include "config.h"
#include "heater/elements/element.hpp"

namespace toothless {

class GPIOElement : public BaseElement {
 public:
  esp_err_t Init() override;
  bool IsOn() const override;

 private:
  gpio_num_t _pin;
  esp_err_t PowerOn(uint8_t duty);
  esp_err_t PowerOff();
};
}  // namespace toothless