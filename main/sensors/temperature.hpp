#pragma once

#include "sensors/sensor.hpp"

namespace toothless {

class Temperature : public Sensor {
public:
  ~Temperature() = default;
  esp_err_t Init();
  esp_err_t Loop();

private:
  bool _initialized = false;
};
} // namespace toothless