#pragma once

// https://github.com/m5stack/M5Unit-ACSSR/blob/main/src/M5_ACSSR.cpp

#include <i2c_master.h>

#include <cstdint>

#include "i2c_manager.hpp"

// I2C
static constexpr uint16_t kACSSRDefaultAddress = 0x50;
static constexpr uint16_t kACSSRRelayRegister = 0x00;
static constexpr uint16_t kACSSRLEDRegister = 0x10;
static constexpr uint16_t kACSSRAddressRegister = 0x20;
static constexpr uint16_t kACSSRVersionRegister = 0xFE;

// #define ACSSR_DEFAULT_ADDR 0x50
// #define ACSSR_I2C_RELAY_REG 0x00
// #define ACSSR_I2C_LED_REG 0x10
// #define ACSSR_I2C_ADDR_REG 0x20
// #define ACSSR_I2C_VERSION_REG 0xFE

// Modbus
// #define ACSSR_DEFAULT_SLAVE_ID 0x04
// #define ACSSR_RELAY_COIL_ADDR 0x0000
// #define ACSSR_LED_HOLDING_ADDR 0x0000
// #define ACSSR_VERSION_HOLDING_ADDR 0x0001
// #define ACSSR_ID_HOLDING_ADDR 0x0002

class M5AcSsr {
 public:
  M5AcSsr();
  // M5AcSsr(i2c_master_dev_handle_t handle);
  esp_err_t SetDeviceAddr(uint8_t addr);
  esp_err_t SetLEDColor(uint32_t color);
  uint8_t GetVersion();
  uint32_t GetLEDColor();
  esp_err_t On();
  esp_err_t Off();
  esp_err_t Status();

 private:
  uint8_t _addr;
  i2c_master_dev_handle_t _dev_handle;
  std::shared_ptr<I2cManager> _i2c_mgr;
  uint8_t _sda;
  uint8_t _scl;
  bool writeBytes(uint8_t addr, uint8_t reg, uint8_t* buffer, uint8_t length);
  bool readBytes(uint8_t addr, uint8_t reg, uint8_t* buffer, uint8_t length);
};
