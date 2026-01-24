#include "impl_expander.hpp"

#include <custom_io_expander_ch32v003.h>

#include "funlog.h"
#include "i2c_manager.hpp"
#include "implementation.hpp"

#if defined(CONFIG_IO_EXPANDER_ENABLE_GPIO_API_WRAPPER)
#include <esp_io_expander_gpio_wrapper.h>
#endif

esp_io_expander_handle_t io_expander = NULL;

namespace impl {
namespace expander {
esp_err_t ExpanderSetup() {
  FLOG_INFO("Setting up IO Expander at address 0x%02X", kIOExpanderAddress);
  i2c_master_bus_handle_t i2c_handle = I2cManager::GetInstance()->GetBusHandle();

  custom_io_expander_new_i2c_ch32v003(i2c_handle, kIOExpanderAddress, &io_expander);
  FLOG_INFO("IO Expander pin state:");
  esp_io_expander_print_state(io_expander);

#if defined(CONFIG_IO_EXPANDER_ENABLE_GPIO_API_WRAPPER)
  FLOG_INFO("Appending GPIO API wrapper to IO Expander");
  esp_io_expander_gpio_wrapper_append_handler(io_expander, GPIO_NUM_MAX);
  gpio_get_level(XP_NUM_07);  // Test read of expander GPIO
#endif

  // esp_io_expander_set_dir(io_expander, kBeeperEnablePin, IO_EXPANDER_OUTPUT);
  // esp_io_expander_set_level(io_expander, kBeeperEnablePin, 1);
  // vTaskDelay(pdMS_TO_TICKS(100));
  // esp_io_expander_set_level(io_expander, kBeeperEnablePin, 0);

  return ESP_OK;
}

esp_io_expander_handle_t GetExpanderHandle() { return io_expander; }

esp_err_t SetPWM(uint8_t duty_cycle) {
  if (duty_cycle > 100) {
    duty_cycle = 100;
  } else if (duty_cycle < 0) {
    duty_cycle = 0;
  }

  int flipped = 100 - duty_cycle;

  uint8_t pwm = (uint8_t)flipped;

  return custom_io_expander_set_pwm(io_expander, pwm * kPwmMax / 100);
}

}  // namespace expander
}  // namespace impl