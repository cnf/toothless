#include "sensors/sensors.hpp"

#include "funlog.h"
#include "sensors/temperature.hpp"

namespace toothless {
esp_err_t Sensors::Init() {
  // _sensor_list.push_back(std::make_shared<Temperature>());
  for (auto& sensor : _sensor_list) {
    sensor->Init();
  }
  return ESP_OK;
}

void Sensors::Loop() {
  for (auto& sensor : _sensor_list) {
    sensor->Loop();
  }
  // FLOG_INFO("LOOP");
  return;
}

}  // namespace toothless