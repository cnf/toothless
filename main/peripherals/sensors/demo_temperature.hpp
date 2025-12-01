// demo_temperature.hpp
#pragma once

#include <memory>

#include "helpers/rolling_average.hpp"
#include "peripherals/peripheral.hpp"

extern "C" {
#include <pubsub.h>
}

namespace toothless {

inline constexpr const char* kDemoTempName = "Demo Temperature";
inline constexpr BusType kDemoTempBusType = BusType::kGPIO;
inline constexpr uint8_t kDemoTempAddress = 0x00;

class DemoTemperature : public Peripheral {
 public:
  static bool Detect() { return true; }  // Always available
  esp_err_t Init() override;
  esp_err_t Loop() override;
  const PeripheralInfo& Info() const override { return _info; }
  static const PeripheralInfo& GetInfo() { return _info; }

  static std::shared_ptr<DemoTemperature> GetInstance() {
    static auto instance = std::make_shared<DemoTemperature>();
    return instance;
  }

 private:
  static const PeripheralInfo _info;
  std::string _topic;
  RollingAverage<uint32_t, kTemperatureAverageSamples> _avg;
  bool _heating = false;
  uint32_t _room_temp = 2200;     // 22.00°C
  uint32_t _current_temp = 2500;  // Start at 25°C (in centi-degrees)
  uint32_t _target_temp = 2500;
  int64_t _last_update = 0;
  ps_subscriber_t* _sub;
};

}  // namespace toothless