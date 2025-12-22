#pragma once

#include "peripherals/peripheral.hpp"

namespace toothless {
class Sensor : public Peripheral {
 public:
  Sensor() = default;
  virtual ~Sensor() = default;
  inline void SetAltTopic(const std::string& topic) { _alt_topic = topic; }
  inline void ClearAltTopic() { _alt_topic.clear(); }

 protected:
  std::string _topic;
  std::string _alt_topic;
};
}  // namespace toothless