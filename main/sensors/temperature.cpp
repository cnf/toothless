#include "sensors/temperature.hpp"

#include <esp_err.h>

#include "config.h"
#include "funlog.h"
#include "sensors/temperature/max6675.hpp"

extern "C" {
#include <pubsub.h>
}

namespace toothless {
esp_err_t Temperature::Init() {
  _initialized = true;
  ESP_ERROR_CHECK(
      Max6675Setup((int8_t)CONFIG_TL_TC_CLK_PIN, (int8_t)CONFIG_TL_TC_CS_PIN, (int8_t)CONFIG_TL_TC_MISO_PIN));
  uint32_t temp;
  Max6675GetTemp(temp);
  _temperature_samples.fill(temp);
  _temperature_average = temp;
  return ESP_OK;
}
esp_err_t Temperature::Loop() {
  static uint64_t count;

  if (!_initialized) {
    return ESP_ERR_INVALID_STATE;
  }
  // TODO: rolling average
  /*
  Beelsebob — 20:23Saturday, November 1, 2025 at 20:23
  keep a vector of n values initialised to 0 and an average value initialised to 0, each frame, subtract vector[f%n]/n
  from the average, put your new value in vector[f % n] where f is the frame number.   And the. Add vector[f%n]/n to the
  average. Where n is the number of frames you’re averaging over
  If you want slightly more complex code but faster initialisation, initialise all vertor values and the average to your
  first reading
  */
  uint32_t temp;
  Max6675GetTemp(temp);
  _temperature_average -= _temperature_samples[count % kTemperatureAverageSamples] / kTemperatureAverageSamples;
  _temperature_samples[count % kTemperatureAverageSamples] = temp;
  _temperature_average += _temperature_samples[count % kTemperatureAverageSamples] / kTemperatureAverageSamples;
  count++;
  PS_PUB_INT("sensor.temperature.chamber", _temperature_average);
  // FLOG_INFO("Thermocouple temperature: %.2f C", temp / 100);
  // FLOG_INFO("Thermocouple reading: %d", temp);
  return ESP_OK;
}
}  // namespace toothless
