#include "max6675.h"
#include "funlog.h"
#include <driver/gpio.h>
#include <unistd.h>

MAX6675::MAX6675(int8_t clock, int8_t chip_select, int8_t data) {
  _clk = clock;
  _cs = chip_select;
  _data = data;

  // define pin modes
  ESP_ERROR_CHECK(gpio_set_direction((gpio_num_t)_cs, GPIO_MODE_OUTPUT));
  ESP_ERROR_CHECK(gpio_set_direction((gpio_num_t)_clk, GPIO_MODE_OUTPUT));
  ESP_ERROR_CHECK(gpio_set_direction((gpio_num_t)_data, GPIO_MODE_INPUT));
  ESP_ERROR_CHECK(gpio_set_level((gpio_num_t)_cs, 1));
  FLOG_INFO("CS: %d, CLK: %d, DATA: %d", _cs, _clk, _data);
}

std::optional<float> MAX6675::ReadCelsius(void) {

  uint16_t v;

  // digitalWrite(_cs, LOW);
  FLOG_DEBUG("Getting Temp, CS: %d", _cs);
  esp_err_t err = gpio_set_level((gpio_num_t)_cs, 0);
  if (err != ESP_OK) {
    FLOG_ERROR("OOPS: %s", esp_err_to_name(err));
  }
  usleep(10);
  // delayMicroseconds(10);

  v = ReadBus();
  v <<= 8;
  v |= ReadBus();

  // digitalWrite(_cs, HIGH);
  err = gpio_set_level((gpio_num_t)_cs, 1);
  if (err != ESP_OK) {
    FLOG_ERROR("OOPS: %s", esp_err_to_name(err));
  }

  if (v & 0x4) {
    // uh oh, no thermocouple attached!
    return std::nullopt; //;NAN;
    // return -100;
  }

  v >>= 3;

  return v * 0.25;
}

std::optional<float> MAX6675::ReadFahrenheit(void) {
  if (auto c = ReadCelsius()) {
    return (*c) * 9.0 / 5.0 + 32;
  }
  return std::nullopt;
  // return ReadCelsius() * 9.0 / 5.0 + 32;
}

uint8_t MAX6675::ReadBus(void) {
  int i;
  uint8_t d = 0;

  for (i = 7; i >= 0; i--) {
    gpio_set_level((gpio_num_t)_clk, 0);
    usleep(10);
    if (gpio_get_level((gpio_num_t)_data)) {
      // set the bit to 0 no matter what
      d |= (1 << i);
    }

    gpio_set_level((gpio_num_t)_clk, 1);
    usleep(10);
  }
  return d;
}
