#pragma once

#include <esp_err.h>

#include <memory>
#include <vector>

#include "topics/topics.hpp"

namespace toothless {

namespace topics::sensor {
static constexpr const char* base = "sensor";
}  // namespace topics::sensor

class Sensor {
 public:
  Sensor() {};
  virtual ~Sensor() = default;
  virtual esp_err_t Init() = 0;
  virtual esp_err_t Loop() = 0;
};
}  // namespace toothless