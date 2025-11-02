#pragma once

#include <esp_err.h>
#include <memory>
#include <vector>

namespace toothless {

class Sensor {
public:
  Sensor() {};
  virtual ~Sensor() = default;
  virtual esp_err_t Init() = 0;
  virtual esp_err_t Loop() = 0;
};
} // namespace toothless