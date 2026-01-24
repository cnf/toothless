#pragma once

#include <esp_err.h>
#include <esp_io_expander.h>

namespace impl {
namespace expander {

// #define LCD_BRIGHTNESS_MAX 0xFF

static constexpr uint16_t kIOExpanderAddress = 0x24;  //<! I2C address of the IO expander
static constexpr uint8_t kPwmMax = 0xFF;              //<! Maximum PWM value
esp_err_t ExpanderSetup();

esp_io_expander_handle_t GetExpanderHandle();

esp_err_t SetPWM(uint8_t duty_cycle);
}  // namespace expander
}  // namespace impl