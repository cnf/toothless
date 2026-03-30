#include "inepro485.hpp"

#include "peripherals/peripheral_registry.hpp"

extern "C" {
#include <pubsub.h>
}

namespace toothless {

const PeripheralInfo InePro::_info = {kIneProName, kTemperature, kIneProBusType, kIneProDefaultAddr};

static bool s_registered = []() {
  PeripheralRegistry::Register({.info = InePro::GetInfo(),
                                .probe = InePro::Detect,
                                .create = []() { return InePro::GetInstance(); },
                                .type = PeripheralRegistry::Registration::Type::kSensor});
  return true;
}();

InePro::InePro() {}

InePro::~InePro() {}

bool InePro::Detect() {
  // Do detection logic here
  return false;
}
esp_err_t InePro::Init() { return esp_err_t(); }
esp_err_t InePro::Loop() { return esp_err_t(); }
}  // namespace toothless