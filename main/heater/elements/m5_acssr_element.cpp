#include "heater/elements/m5_acssr_element.hpp"

#include <driver/gpio.h>
#include <esp_err.h>

#include "funlog.h"

extern "C" {
#include <pubsub.h>
}

namespace toothless {

esp_err_t M5I2CElement::Init() {
  _i2c_mgr = I2cManager::GetInstance();

  return ESP_OK;
};

bool M5I2CElement::IsOn() const { return false; }

esp_err_t M5I2CElement::PowerOn(uint8_t duty) {
  // TODO: PWM support
  PS_PUB_BOOL_FL("heater.power", true, PS_FL_STICKY);
  // return gpio_set_level(_pin, 1);
  return ESP_OK;
}

esp_err_t M5I2CElement::PowerOff() {
  PS_PUB_BOOL_FL("heater.power", false, PS_FL_STICKY);
  // return gpio_set_level(_pin, 0);
  return ESP_OK;
}

}  // namespace toothless