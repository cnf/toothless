#pragma once

#include <esp_err.h>
#include <optional>
#include <stdint.h>
namespace toothless {

esp_err_t Max6675Setup(int8_t clock, int8_t chip_select, int8_t data);

esp_err_t Max6675GetCelsius(float &celsius);
esp_err_t Max6675GetTemp(uint32_t &celsius);
uint8_t Max6675Read();

} // namespace toothless