#pragma once

#include <memory>
#include <vector>

#include "config_mgr.hpp"
#include "i2c_manager.hpp"
#include "sensors/sensor.hpp"

namespace toothless {

namespace topics::sensors {
inline constexpr char const* const name = "sensor";
inline constexpr char const* const temperature = "sensor.temperature";
inline constexpr char const* const humidity = "sensor.humidity";
inline constexpr char const* const current = "sensor.current";

}  // namespace topics::sensors

struct SensorsConfig {};

inline ConfigEntries config_entries = {
    // ConfigEntry("mode", "Default Mode", MakeFormat(), std::string("reflow"), ""),
};

class Sensors {
 public:
  Sensors() {};
  esp_err_t Init();
  void Loop();

 private:
  std::shared_ptr<I2cManager> _i2c_manager;
  std::vector<std::shared_ptr<Sensor>> _sensor_list;
};
}  // namespace toothless