#pragma once

#include <esp_err.h>

#include <cstdint>
#include <memory>

#include "helpers/rolling_average.hpp"
#include "i2c_manager.hpp"
#include "peripherals/peripheral.hpp"
#include "peripherals/sensors/sensor.hpp"

namespace toothless {
// #define KMETER_DEFAULT_ADDR 0x66
// #define KMETER_TEMP_VAL_REG 0x00
// #define KMETER_INTERNAL_TEMP_VAL_REG 0x10
// #define KMETER_KMETER_ERROR_STATUS_REG 0x20
// #define KMETER_TEMP_CELSIUS_STRING_REG 0x30
// #define KMETER_TEMP_FAHRENHEIT_STRING_REG 0x40
// #define KMETER_INTERNAL_TEMP_CELSIUS_STRING_REG 0x50
// #define KMETER_INTERNAL_TEMP_FAHRENHEIT_STRING_REG 0x60
// #define KMETER_FIRMWARE_VERSION_REG 0xFE
// #define KMETER_I2C_ADDRESS_REG 0xFF

static constexpr uint8_t kMeterDefaultAddr = 0x66;
static constexpr uint8_t kMeterRegTemperatureValue = 0x00;
static constexpr uint8_t kMeterRegInternalTemperatureValue = 0x10;
static constexpr uint8_t kMeterRegErrorStatus = 0x20;
static constexpr uint8_t kMeterRegTemperatureCelsiusString = 0x30;
static constexpr uint8_t kMeterRegTemperatureFahrenheitString = 0x40;
static constexpr uint8_t kMeterRegInternalTemperatureCelsiusString = 0x50;
static constexpr uint8_t kMeterRegInternalTemperatureFahrenheitString = 0x60;
static constexpr uint8_t kMeterRegFirmwareVersion = 0xFE;
static constexpr uint8_t kMeterRegI2CAddress = 0xFF;

static constexpr uint32_t kTemperatureReadIntervalMs = 250;
static constexpr BusType kM5KMeterBusType = BusType::kI2C;
static constexpr char kM5KMeterName[] = "M5 K-Meter";

namespace topics::sensors::m5_kmeter {}

class M5KMeter : public Sensor {
 public:
  M5KMeter();
  ~M5KMeter();
  static bool Detect();
  esp_err_t Init() override;
  esp_err_t Loop() override;
  const PeripheralInfo& Info() const override { return _info; }
  static const PeripheralInfo& GetInfo() { return _info; }
  esp_err_t ReadCelsius(uint32_t& celsius);
  // esp_err_t ReadFahrenheit(float& fahrenheit);
  static std::shared_ptr<M5KMeter> GetInstance() {
    static std::shared_ptr<M5KMeter> instance(new M5KMeter());
    return instance;
  }

 private:
  i2c_master_dev_handle_t _dev_handle;
  std::shared_ptr<I2cManager> _i2c_mgr;
  static const PeripheralInfo _info;
  RollingAverage<uint32_t, kTemperatureAverageSamples> _avg;
  uint8_t _error_counter = 0;
};
}  // namespace toothless