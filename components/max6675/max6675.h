#pragma once

#include <optional>
#include <stdint.h>

class MAX6675 {
public:
  MAX6675(int8_t clock, int8_t _cs, int8_t data);

  std::optional<float> ReadCelsius(void);
  std::optional<float> ReadFahrenheit(void);

private:
  int8_t _clk, _data, _cs;
  uint8_t ReadBus(void);
};