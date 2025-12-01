#pragma once

#include <esp_err.h>

#include <cstdint>

namespace toothless {

enum class BusType { kI2C, kSPI, kGPIO };

enum SensorType { kTemperature, kHumidity, kOtherSensor };
enum ActuatorType { kHeater, kCooler, kFan, kOtherActuator };

inline const char* BusTypeToString(BusType bus) {
  switch (bus) {
    case BusType::kI2C:
      return "i2c";
    case BusType::kSPI:
      return "spi";
    case BusType::kGPIO:
      return "spio";
    default:
      return "Unknown";
  }
}

struct PeripheralInfo {
  const char* name;  // "M5 K-Meter"
  const char* type;  // "temperature"
  BusType bus;
  uint8_t address;  // For I2C, 0 for non-I2C
};

inline constexpr size_t kTemperatureAverageSamples = 10;

class Peripheral {
 public:
  virtual ~Peripheral() = default;
  // virtual bool Detect() = 0; // Can't be static if virtual
  virtual esp_err_t Init() = 0;
  virtual esp_err_t Loop() = 0;
  virtual const PeripheralInfo& Info() const = 0;

 protected:
  bool _initialized = false;
};

}  // namespace toothless