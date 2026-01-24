// demo_temperature.cpp
#include "demo_temperature.hpp"

#include <esp_random.h>
#include <esp_timer.h>

#include <algorithm>

#include "funlog.h"
#include "heater/heater.hpp"
#include "helpers/string_to_snake.hpp"
#include "peripherals/peripheral_registry.hpp"

extern "C" {
#include <pubsub.h>
}

namespace toothless {

const PeripheralInfo DemoTemperature::_info = {kDemoTempName, kTemperature, kDemoTempBusType, kDemoTempAddress};

static bool s_registered = []() {
  PeripheralRegistry::Register({.info = DemoTemperature::GetInfo(),
                                .probe = DemoTemperature::Detect,
                                .create = []() { return DemoTemperature::GetInstance(); },
                                .type = PeripheralRegistry::Registration::Type::kSensor});
  return true;
}();

esp_err_t DemoTemperature::Init() {
  _topic = std::format("{}.{}.{}", topics::peripherals::sensors::temperature, BusTypeToString(kDemoTempBusType),
                       StringToSnake(kDemoTempName));

  _initialized = true;
  _last_update = esp_timer_get_time();
  FLOG_DEBUG("Demo Temperature initialized, topic: %s", _topic.c_str());
  _sub = ps_new_subscriber(10, PS_STRLIST(topics::heater::target, topics::heater::state));

  return ESP_OK;
}

esp_err_t DemoTemperature::Loop() {
  if (!_initialized) return ESP_ERR_INVALID_STATE;

  ps_msg_t* msg = nullptr;
  for ((msg = ps_get(_sub, 0)); msg != NULL; (msg = ps_get(_sub, 0))) {
    if (ps_has_topic(msg, topics::heater::target) && PS_IS_INT(msg)) {
      _target_temp = msg->int_val;
    } else if (ps_has_topic(msg, topics::heater::state) && PS_IS_INT(msg)) {
      if (msg->int_val == heater::kStateOn) {
        _heating = true;
      } else {
        _heating = false;
      }
    }
    ps_unref_msg(msg);
  }

  int64_t now = esp_timer_get_time();
  int64_t dt_ms = (now - _last_update) / 1000;
  _last_update = now;

  // Simulate thermal dynamics: drift toward target
  int32_t diff;
  int32_t change;
  if (_heating) {
    // Heating: approach target temp
    // Simple first-order system: dT/dt = (T_target - T_current) / tau
    // Discretized: delta_T = (T_target - T_current) * (dt / tau)
    // Let's assume tau = 5s for heating
    diff = static_cast<int32_t>(_target_temp) - static_cast<int32_t>(_current_temp);
    change = (diff * dt_ms) / 5000;  // ~5s time constant
  } else {
    // Cooling: drift toward room temp
    diff = static_cast<int32_t>(_room_temp) - static_cast<int32_t>(_current_temp);
    change = (diff * dt_ms) / 15000;  // ~15s time constant
  }

  // Add noise (±500 milli-degrees)
  int32_t noise = (esp_random() % 100) - 50;

  _current_temp = static_cast<uint32_t>(std::max<int32_t>(0, static_cast<int32_t>(_current_temp) + change + noise));

  _avg.Add(_current_temp);
  PS_PUB_INT(_topic.c_str(), _avg.Get());
  if (!_alt_topic.empty()) {
    PS_PUB_INT(_alt_topic.c_str(), _avg.Get());
  }
  // PS_PUB_INT("sensor.temperature.zone", _avg.Get());
  // PS_PUB_INT("sensor.temperature.", _avg.Get());
  return ESP_OK;
}

}  // namespace toothless