#include <driver/ledc.h>

#include "display_impl.hpp"
#include "funlog.h"

namespace impl {
namespace display {

esp_err_t BacklightSetup() {
  FLOG_INFO("Setting up backlight control...");
  // Set up LEDC for backlight PWM control
  ledc_timer_config_t ledc_timer = {.speed_mode = LEDC_LOW_SPEED_MODE,
                                    .duty_resolution = LEDC_TIMER_10_BIT,
                                    .timer_num = LEDC_TIMER_0,
                                    .freq_hz = 5000,
                                    .clk_cfg = LEDC_AUTO_CLK};
  ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));

  ledc_channel_config_t ledc_channel = {.gpio_num = (gpio_num_t)kLcdBacklightPin,
                                        .speed_mode = LEDC_LOW_SPEED_MODE,
                                        .channel = LEDC_CHANNEL_0,
                                        .intr_type = LEDC_INTR_DISABLE,
                                        .timer_sel = LEDC_TIMER_0,
                                        .duty = 1023,  // Start with backlight on
                                        .hpoint = 0};
  ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));
  return ESP_OK;
}

esp_err_t SetBrightness(uint8_t brightness) {
  // return ESP_OK;
  // brightness: 0-100
  if (brightness > 100) brightness = 100;
  uint32_t duty = (brightness * 1023) / 100;  // Scale to 0-1023
  ESP_ERROR_CHECK(ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, duty));
  ESP_ERROR_CHECK(ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0));
  return ESP_OK;
}

}  // namespace display
}  // namespace impl