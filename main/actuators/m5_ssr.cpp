#include <m5_ssr.hpp>
#include "i2c_manager.hpp

// bool M5_ACSSR::begin(TwoWire* wire, uint8_t sda, uint8_t scl, uint8_t addr) {
//   _wire = wire;
//   _addr = addr;
//   _sda = sda;
//   _scl = scl;
//   _wire->begin((int)_sda, (int)_scl, 100000UL);
//   delay(10);
//   _wire->beginTransmission(_addr);
//   uint8_t error = _wire->endTransmission();
//   if (error == 0) {
//     return true;
//   } else {
//     return false;
//   }
// }

M5AcSsr::M5AcSsr() {
  _i2c_mgr = I2cManager::GetInstance();
  i2c_device_config_t device_config = {
      .dev_addr_length = I2C_ADDR_BIT_LEN_7,
      .device_address = kACSSRDefaultAddress,
      .scl_speed_hz = I2cManager::kClockSpeedHz,
  };
  ESP_ERROR_CHECK_WITHOUT_ABORT(_i2c_mgr->AddDevice(&device_config, &_dev_handle));
}

bool M5AcSsr::writeBytes(uint8_t addr, uint8_t reg, uint8_t* buffer, uint8_t length) {
  //   _wire->beginTransmission(addr);
  //   _wire->write(reg);
  //   _wire->write(buffer, length);
  //   if (_wire->endTransmission() == 0) return true;
  //   return false;
}

bool M5AcSsr::readBytes(uint8_t addr, uint8_t reg, uint8_t* buffer, uint8_t length) {
  //   uint8_t index = 0;
  //   _wire->beginTransmission(addr);
  //   _wire->write(reg);
  //   _wire->endTransmission();
  //   if (_wire->requestFrom(addr, length)) {
  //     for (uint8_t i = 0; i < length; i++) {
  //       buffer[index++] = _wire->read();
  //     }
  //     return true;
  //   }
  //   return false;
}

// M5AcSsr::M5AcSsr(i2c_master_dev_handle_t handle) {
//   _device_handle = handle;
//   _addr = kACSSRDefaultAddress;
// }

esp_err_t M5AcSsr::On() {
  uint8_t data = 1;
  return _i2c_mgr->WriteRegister(_dev_handle, kACSSRRelayRegister, &data, 1);
}

esp_err_t M5AcSsr::Off() {
  uint8_t data = 0;
  return _i2c_mgr->WriteRegister(_dev_handle, kACSSRRelayRegister, &data, 1);
}

esp_err_t M5AcSsr::Status() {
  uint8_t data = 0;
  _i2c_mgr->WriteRegister(_dev_handle, kACSSRRelayRegister, &data, 1);
  return data;
}

esp_err_t M5AcSsr::SetLEDColor(uint32_t colorHEX) {
  uint8_t color[3];
  // RED
  color[0] = (colorHEX >> 16) & 0xff;
  // GREEN
  color[1] = (colorHEX >> 8) & 0xff;
  // BLUE
  color[2] = colorHEX & 0xff;
  return _i2c_mgr->WriteRegister(_dev_handle, kACSSRLEDRegister, color, 3);
}

uint32_t M5AcSsr::GetLEDColor() {
  uint8_t color[3];
  uint32_t colorHEX = 0;
  readBytes(_addr, kACSSRLEDRegister, color, 3);
  colorHEX = color[0];
  colorHEX = (colorHEX << 8) | color[1];
  colorHEX = (colorHEX << 8) | color[2];
  return colorHEX;
}

bool M5AcSsr::SetDeviceAddr(uint8_t addr) {
  uint8_t data = addr;
  if (_i2c_mgr->WriteRegister(_dev_handle, kACSSRAddressRegister, &data, 1)) {
    _addr = addr;
    return true;
  } else {
    return false;
  }
}

/*! @brief Get the Version of Firmware.
    @return Firmware version */
uint8_t M5AcSsr::GetVersion() {
  uint8_t data = 0;
  _i2c_mgr->WriteRegister(_dev_handle, kACSSRVersionRegister, &data, 1);
  return data;
}