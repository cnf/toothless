#pragma once

#include <esp_err.h>

#include <cstdint>
#include <string>
#include <variant>

namespace toothless {

enum class BusType { kI2C, kSPI, kGPIO, kModBus };

inline const char* BusTypeToString(BusType bus) {
  switch (bus) {
    case BusType::kI2C:
      return "i2c";
    case BusType::kSPI:
      return "spi";
    case BusType::kGPIO:
      return "gpio";
    case BusType::kModBus:
      return "modbus";
    default:
      return "Unknown";
  }
}

enum SensorType { kTemperature, kHumidity, kCurrent, kVolt, kOtherSensor };

inline const char* SensorTypeToString(SensorType type) {
  switch (type) {
    case kTemperature:
      return "temperature";
    case kHumidity:
      return "humidity";
    case kCurrent:
      return "current";
    case kVolt:
      return "volt";
    case kOtherSensor:
      return "other";
    default:
      return "unknown";
  }
}

enum ActuatorType { kSSR, kHeater, kCooler, kFan, kOtherActuator };

inline const char* ActuatorTypeToString(ActuatorType type) {
  switch (type) {
    case kSSR:
      return "ssr";
    case kHeater:
      return "heater";
    case kCooler:
      return "cooler";
    case kFan:
      return "fan";
    case kOtherActuator:
      return "other";
    default:
      return "unknown";
  }
}

struct PeripheralInfo {
  const char* name;  // "M5 K-Meter"
  // const char* type;  // "temperature"
  std::variant<SensorType, ActuatorType> type;
  BusType bus;
  uint8_t address;  // For I2C, ModBus, etc.
};

inline constexpr size_t kTemperatureAverageSamples = 10;

class Peripheral {
 public:
  virtual ~Peripheral() = default;
  // virtual bool Detect() = 0; // Can't be static if virtual
  virtual esp_err_t Init() = 0;
  virtual esp_err_t Loop() = 0;
  virtual const PeripheralInfo& Info() const = 0;
  inline std::string Topic() const { return _topic; }
  // inline void SetAltTopic(const std::string& topic) { _alt_topic = topic; }
  // inline void ClearAltTopic() { _alt_topic.clear(); }

 protected:
  bool _initialized = false;
  std::string _topic;
  // std::string _alt_topic;
};

}  // namespace toothless