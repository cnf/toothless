#pragma once

// https://github.com/m5stack/M5Unit-ACSSR/blob/main/src/M5_ACSSR.cpp

#include <i2c_master.h>

#include <cstdint>

// I2C
#define ACSSR_DEFAULT_ADDR 0x50
#define ACSSR_I2C_RELAY_REG 0x00
#define ACSSR_I2C_LED_REG 0x10
#define ACSSR_I2C_ADDR_REG 0x20
#define ACSSR_I2C_VERSION_REG 0xFE

// Modbus
#define ACSSR_DEFAULT_SLAVE_ID 0x04
#define ACSSR_RELAY_COIL_ADDR 0x0000
#define ACSSR_LED_HOLDING_ADDR 0x0000
#define ACSSR_VERSION_HOLDING_ADDR 0x0001
#define ACSSR_ID_HOLDING_ADDR 0x0002

class M5AcSsr {
 public:
  M5AcSsr(i2c_master_dev_handle_t handle);
  // bool begin(TwoWire* wire = &Wire, uint8_t sda = SDA, uint8_t scl = SCL, uint8_t addr = ACSSR_DEFAULT_ADDR);
  bool SetDeviceAddr(uint8_t addr);
  bool SetLEDColor(uint32_t color);
  uint8_t GetVersion();
  uint32_t GetLEDColor();
  bool On();
  bool Off();
  bool Status();

 private:
  uint8_t _addr;
  i2c_master_dev_handle_t _device_handle;
  uint8_t _sda;
  uint8_t _scl;
  bool writeBytes(uint8_t addr, uint8_t reg, uint8_t* buffer, uint8_t length);
  bool readBytes(uint8_t addr, uint8_t reg, uint8_t* buffer, uint8_t length);
};
