#include "heater/elements/gpio_element.hpp"

#include <driver/gpio.h>
#include <esp_err.h>

#include "funlog.h"

extern "C" {
#include <pubsub.h>
}

namespace toothless {

esp_err_t GPIOElement::Init() {
  _pin = (gpio_num_t)kHeaterControlPin;
  ESP_ERROR_CHECK(gpio_set_direction(_pin, GPIO_MODE_OUTPUT));
  ESP_ERROR_CHECK(gpio_set_level(_pin, 0));
  return ESP_OK;
};

bool GPIOElement::IsOn() const { return gpio_get_level(_pin); }

esp_err_t GPIOElement::PowerOn(uint8_t duty) {
  // TODO: PWM support
  PS_PUB_BOOL_FL("heater.power", true, PS_FL_STICKY);
  return gpio_set_level(_pin, 1);
}

esp_err_t GPIOElement::PowerOff() {
  PS_PUB_BOOL_FL("heater.power", false, PS_FL_STICKY);
  return gpio_set_level(_pin, 0);
}

}  // namespace toothless