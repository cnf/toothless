#pragma once

namespace toothless {
// Peripheral management namespace
namespace peripheral {

enum Protocol { kI2C, kSPI, kOneWire, kAnalog };

enum SensorType { kTemperature, kHumidity, kOtherSensor };
enum ActuatorType { kHeater, kCooler, kFan, kOtherActuator };

struct ProtocolConfig {
  // Base protocol configuration
};
};  // namespace peripheral
};  // namespace toothless