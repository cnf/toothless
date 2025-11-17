#pragma once

#include <driver/gpio.h>
#include <esp_types.h>

#include <cstddef>

namespace display {
namespace impl {
static constexpr size_t kHRes = CONFIG_IMPL_LILYGO_T_HMI_HRES;  //<! horizontal resolution
static constexpr size_t kVRes = CONFIG_IMPL_LILYGO_T_HMI_VRES;  //<! vertical resolution

static constexpr gpio_num_t kPowerEnablePin = GPIO_NUM_10;  //<! power enable pin
static constexpr gpio_num_t kPowerOnPin = GPIO_NUM_14;      //<! power on pin
static constexpr gpio_num_t kBatteryAdcPin = GPIO_NUM_5;    //<! battery adc pin
static constexpr gpio_num_t kButton1Pin = GPIO_NUM_0;       //<! button 1 pin
static constexpr gpio_num_t kButton2Pin = GPIO_NUM_21;      //<! button 2 pin
static constexpr gpio_num_t kLedPin = GPIO_NUM_15;          //<! led pin

}  // namespace impl
}  // namespace display