#include "peripherals/actuators/m5_acssr_element.hpp"

#include <driver/gpio.h>
#include <esp_err.h>

#include "funlog.h"
#include "peripherals/peripheral.hpp"
#include "peripherals/peripheral_registry.hpp"

extern "C" {
#include <pubsub.h>
}

namespace toothless {

const PeripheralInfo M5I2CElement::_info = {kACSSRName, kSSR, kACSSRBusType, kACSSRDefaultAddress};

static bool s_registered = []() {
  PeripheralRegistry::Register({.info = M5I2CElement::GetInfo(),
                                .probe = M5I2CElement::Detect,
                                .create = []() { return M5I2CElement::GetInstance(); },
                                .type = PeripheralRegistry::Registration::Type::kActuator});
  return true;
}();

esp_err_t M5I2CElement::Init() {
  _i2c_mgr = I2cManager::GetInstance();
  i2c_device_config_t device_config = {
      .dev_addr_length = I2C_ADDR_BIT_LEN_7,
      .device_address = kACSSRDefaultAddress,
      .scl_speed_hz = 100 * 1000,
  };
  ESP_ERROR_CHECK_WITHOUT_ABORT(_i2c_mgr->AddDevice(&device_config, &_dev_handle));
  FLOG_DEBUG("M5 AC SSR Element initialized, version: %d", Version());

  return PowerOff();
};

bool M5I2CElement::IsOn() const {
  return _is_on;
  // uint8_t data = 0;
  // _i2c_mgr->WriteRegister(_dev_handle, kACSSRRelayRegister, &data, 1);
  // return data;
}

esp_err_t M5I2CElement::Loop() { return ESP_OK; }

const PeripheralInfo& M5I2CElement::Info() const { return _info; }

bool M5I2CElement::Detect() {
  // BUG: this isn't reliable, figure out a better way to detect
  esp_err_t err = I2cManager::GetInstance()->Probe(kACSSRDefaultAddress);
  if (err == ESP_OK) {
    FLOG_DEBUG("M5 AC SSR Element detected at address 0x%02X", kACSSRDefaultAddress);
    return true;
  } else {
    FLOG_DEBUG("M5 AC SSR Element not detected at address 0x%02X: %s", kACSSRDefaultAddress, esp_err_to_name(err));
  }
  return false;
}

esp_err_t M5I2CElement::PowerOn(uint8_t duty) {
  _is_on = true;
  return Power(true);
  // TODO: PWM support
}

esp_err_t M5I2CElement::PowerOff() {
  _is_on = false;
  return Power(false);
}

esp_err_t M5I2CElement::Power(bool on) {
  uint8_t data = (uint8_t)on;
  ESP_ERROR_CHECK_WITHOUT_ABORT(_i2c_mgr->WriteRegister(_dev_handle, kACSSRRelayRegister, &data, 1));
  SetLEDColor(on ? 0xFF0000 : 0x000022);  //<!
  PS_PUB_BOOL_FL("heater.power", on, PS_FL_STICKY);
  return ESP_OK;
}
uint8_t M5I2CElement::Version() {
  uint8_t data = 0;
  // _i2c_mgr->WriteRegister(_dev_handle, kACSSRVersionRegister, &data, 1);
  _i2c_mgr->ReadRegister(_dev_handle, kACSSRVersionRegister, &data, 1);

  return data;
};

esp_err_t M5I2CElement::SetLEDColor(uint32_t colorHEX) {
  uint8_t color[3];
  // RED
  color[0] = (colorHEX >> 16) & 0xff;
  // GREEN
  color[1] = (colorHEX >> 8) & 0xff;
  // BLUE
  color[2] = colorHEX & 0xff;
  return _i2c_mgr->WriteRegister(_dev_handle, kACSSRLEDRegister, color, 3);
}

}  // namespace toothless