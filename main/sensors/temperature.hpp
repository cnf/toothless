#pragma once

#include "sensors/sensor.hpp"
#include "topics/topics.hpp"

namespace toothless {
inline constexpr size_t kTemperatureAverageSamples = 10;

namespace topics::sensor::temperature {
inline constexpr char const* const base = "sensor.temperature.chamber";
}  // namespace topics::sensor::temperature

class Temperature : public Sensor {
 public:
  ~Temperature() = default;
  esp_err_t Init();
  esp_err_t Loop();

 private:
  bool _initialized = false;
  std::array<uint32_t, kTemperatureAverageSamples> _temperature_samples;
  uint32_t _temperature_average = 0;
};
}  // namespace toothless