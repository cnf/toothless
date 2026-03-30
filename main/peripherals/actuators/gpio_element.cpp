#include "gpio_element.hpp"

#include <driver/gpio.h>
#include <esp_err.h>

#include "funlog.h"
#include "implementation.hpp"
#include "peripherals/peripheral.hpp"
#include "peripherals/peripheral_registry.hpp"

extern "C" {
#include <pubsub.h>
}

namespace toothless {

const PeripheralInfo GPIOElement::_info = {kGPIOElementName, kSSR, kGPIOElementBusType, kGPIOElementAddress};

static bool s_registered = []() {
  PeripheralRegistry::Register({.info = GPIOElement::GetInfo(),
                                .probe = GPIOElement::Detect,
                                .create = []() { return GPIOElement::GetInstance(); },
                                .type = PeripheralRegistry::Registration::Type::kActuator,
                                .pin_slots = {{"ctrl_pin", "SSR Control Pin"}}});

  return true;
}();

bool GPIOElement::Detect() {
  // Can't drive GPIO if no GPIOs are available
  if (sizeof(impl::kGpioFreeList) / sizeof(impl::kGpioFreeList[0]) == 0 || impl::kGpioFreeList[0] == GPIO_NUM_NC) {
    return false;
  }
  return true;
}

esp_err_t GPIOElement::Init() {
  _config = std::make_shared<SettingsMap>();
  _config_entries = std::make_unique<ConfigEntries>();
  _config_entries->insert(std::end(*_config_entries), std::begin(gpio_config_entries), std::end(gpio_config_entries));
  RegisterConfig(_config_entries.get(), topics::peripherals::gpio_ssr::name);
  FLOG_DEBUG("Waiting for GPIO Element settings...");
  GetSettings(_config, topics::peripherals::gpio_ssr::name);
  FLOG_DEBUG("GPIO Element settings loaded");
  auto gpio_res = PeripheralRegistry::GpioFromString(std::get<std::string>(_config->at("ctrl_pin")));
  if (!gpio_res) {
    FLOG_ERROR("Invalid GPIO pin for GPIO Element");
    return gpio_res.error();
  }
  _pin = gpio_res.value();
  // // _pin = (gpio_num_t)std::get<int>(_config->at("ctrl_pin"));
  // if (std::holds_alternative<std::string>(_config->at("ctrl_pin"))) {
  //   std::string pin_str = std::get<std::string>(_config->at("ctrl_pin"));
  //   FLOG_INFO("GPIO Element control pin: %s", pin_str.c_str());
  // } else {
  //   FLOG_ERROR("GPIO Element control pin has wrong type");
  // };
  // std::string pin_str = std::get<std::string>(_config->at("ctrl_pin"));
  if (_pin == GPIO_NUM_NC) {
    FLOG_ERROR("GPIO Element control pin not configured");
    return ESP_ERR_INVALID_ARG;
  }
  // gpio_set_direction(_pin, GPIO_MODE_OUTPUT);
  // _pin = (gpio_num_t)kHeaterControlPin;
  ESP_ERROR_CHECK(gpio_set_direction(_pin, GPIO_MODE_OUTPUT));
  ESP_ERROR_CHECK(gpio_set_level(_pin, 0));
  return ESP_OK;
};

esp_err_t GPIOElement::Loop() { return ESP_OK; }

const PeripheralInfo& GPIOElement::Info() const { return _info; }

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