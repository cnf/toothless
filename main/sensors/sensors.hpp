#pragma once

#include "sensors/sensor.hpp"
#include <memory>
#include <vector>

namespace toothless {

class Sensors {
public:
  Sensors() {};
  esp_err_t Init();
  void Loop();

private:
  std::vector<std::shared_ptr<Sensor>> _sensor_list;
};
} // namespace toothless