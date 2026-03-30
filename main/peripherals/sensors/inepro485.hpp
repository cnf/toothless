#pragma once

#include <esp_err.h>

#include <cstdint>
#include <memory>

// #include "helpers/rolling_average.hpp"
// #include "i2c_manager.hpp"
#include "peripherals/peripheral.hpp"
#include "peripherals/sensors/sensor.hpp"

namespace toothless {

static constexpr uint8_t kIneProDefaultAddr = 0x01;
// static constexpr uint8_t kMeterRegTemperatureValue = 0x00;
// static constexpr uint8_t kMeterRegInternalTemperatureValue = 0x10;
// static constexpr uint8_t kMeterRegErrorStatus = 0x20;
// static constexpr uint8_t kMeterRegTemperatureCelsiusString = 0x30;
// static constexpr uint8_t kMeterRegTemperatureFahrenheitString = 0x40;
// static constexpr uint8_t kMeterRegInternalTemperatureCelsiusString = 0x50;
// static constexpr uint8_t kMeterRegInternalTemperatureFahrenheitString = 0x60;
// static constexpr uint8_t kMeterRegFirmwareVersion = 0xFE;
// static constexpr uint8_t kMeterRegI2CAddress = 0xFF;

// static constexpr uint32_t kTemperatureReadIntervalMs = 250;
static constexpr BusType kIneProBusType = BusType::kModBus;
static constexpr char kIneProName[] = "InePro";

namespace topics::sensors::inepro {}

class InePro : public Sensor {
 public:
  InePro();
  ~InePro();
  static bool Detect();
  esp_err_t Init() override;
  esp_err_t Loop() override;
  const PeripheralInfo& Info() const override { return _info; }
  static const PeripheralInfo& GetInfo() { return _info; }
  static std::shared_ptr<InePro> GetInstance() {
    static std::shared_ptr<InePro> instance(new InePro());
    return instance;
  }

 private:
  // i2c_master_dev_handle_t _dev_handle;
  // std::shared_ptr<I2cManager> _i2c_mgr;
  static const PeripheralInfo _info;
  // RollingAverage<uint32_t, kTemperatureAverageSamples> _avg;
  uint8_t _error_counter = 0;
};
}  // namespace toothless