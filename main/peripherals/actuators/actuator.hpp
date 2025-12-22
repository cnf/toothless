#pragma once

#include "peripherals/peripheral.hpp"

extern "C" {
#include <pubsub.h>
}

namespace toothless {
//
class Actuator : public Peripheral {
 public:
  Actuator() = default;
  virtual ~Actuator() = default;

  // virtual esp_err_t SetValue(float value) = 0;
  virtual esp_err_t Init() = 0;
  inline virtual esp_err_t On(uint8_t duty) final {
    PS_PUB_BOOL_FL("heater.power", true, PS_FL_STICKY);
    return PowerOn(duty);
  };
  inline virtual esp_err_t Off() final {
    PS_PUB_BOOL_FL("heater.power", false, PS_FL_STICKY);
    return PowerOff();
  };
  virtual bool IsOn() const = 0;

 protected:
  uint32_t _pwm_frequency;
  virtual esp_err_t PowerOn(uint8_t duty) = 0;
  virtual esp_err_t PowerOff() = 0;
};
}  // namespace toothless