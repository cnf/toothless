#pragma once

namespace toothless {
// Peripheral management namespace
namespace peripheral {

enum Protocol { kI2C, kSPI, kOneWire, kAnalog };

enum SensorType { kTemperature, kHumidity, kOther };
enum ActuatorType { kHeater, kCooler, kFan, kOther };

struct ProtocolConfig {
  // Base protocol configuration
};
};  // namespace peripheral
};  // namespace toothless