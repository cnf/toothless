#include "sensors/sensors.hpp"

#include "funlog.h"
#include "sensors/temperature.hpp"

extern "C" {
#include <pubsub.h>
}

namespace toothless {
esp_err_t Sensors::Init() {
  // _sensor_list.push_back(std::make_shared<Temperature>());
  for (auto& sensor : _sensor_list) {
    esp_err_t err = sensor->Init();
  }
  return ESP_OK;
}

void Sensors::Loop() {
  static bool toggle;
  toggle = !toggle;
  for (auto& sensor : _sensor_list) {
    sensor->Loop();
  }
  PS_PUB_INT("sensor.temperature.chamber", (120 + uint8_t(toggle)) * 100);  // BUG: just for testing
  // FLOG_INFO("LOOP");
  return;
}

}  // namespace toothless