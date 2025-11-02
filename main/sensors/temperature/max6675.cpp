#include "sensors/temperature/max6675.hpp"
#include "config.h"
#include "funlog.h"
#include "max6675.hpp"
#include <driver/gpio.h>
#include <unistd.h>

static gpio_num_t _max6675_clock;
static gpio_num_t _max6675_chip_select;
static gpio_num_t _max6675_data;

namespace toothless {

esp_err_t Max6675Setup(int8_t clock, int8_t chip_select, int8_t data) {
  _max6675_clock = (gpio_num_t)clock;
  _max6675_chip_select = (gpio_num_t)chip_select;
  _max6675_data = (gpio_num_t)data;
  ESP_ERROR_CHECK(gpio_set_direction((gpio_num_t)_max6675_chip_select, GPIO_MODE_OUTPUT));
  ESP_ERROR_CHECK(gpio_set_direction((gpio_num_t)_max6675_clock, GPIO_MODE_OUTPUT));
  ESP_ERROR_CHECK(gpio_set_direction((gpio_num_t)_max6675_data, GPIO_MODE_INPUT));
  ESP_ERROR_CHECK(gpio_set_level((gpio_num_t)_max6675_chip_select, 1));
  FLOG_INFO("CS: %d, CLK: %d, DATA: %d", _max6675_chip_select, _max6675_clock, _max6675_data);
  return ESP_OK;
}

esp_err_t Max6675GetCelsius(float &celsius) {
  uint32_t temp;
  Max6675GetTemp(temp);
  celsius = temp / 100;
  return ESP_OK;
}

esp_err_t Max6675GetTemp(uint32_t &celsius) {
  uint32_t v;

  FLOG_DEBUG("Getting Temp, CS: %d", _max6675_chip_select);
  esp_err_t err = gpio_set_level((gpio_num_t)_max6675_chip_select, 0);
  if (err != ESP_OK) {
    FLOG_ERROR("OOPS: %s", esp_err_to_name(err));
  }
  usleep(10);

  v = Max6675Read();
  v <<= 8;
  v |= Max6675Read();

  err = gpio_set_level((gpio_num_t)_max6675_chip_select, 1);
  if (err != ESP_OK) {
    FLOG_ERROR("OOPS: %s", esp_err_to_name(err));
  }

  if (v & 0x4) {
    // return ESP_ERR_INVALID_STATE; // no thermocouple attached
    return ESP_ERR_NOT_FOUND; // no thermocouple attached
  }

  v >>= 3;

  celsius = v * 25;
  return ESP_OK;
}

uint8_t Max6675Read() {
  int i;
  uint8_t d = 0;

  for (i = 7; i >= 0; i--) {
    gpio_set_level((gpio_num_t)_max6675_clock, 0);
    usleep(10);
    if (gpio_get_level((gpio_num_t)_max6675_data)) {
      d |= (1 << i);
    }

    gpio_set_level((gpio_num_t)_max6675_clock, 1);
    usleep(10);
  }
  return d;
}

}; // namespace toothless