#pragma once
#include "config.h"

#include "i2c_manager.hpp"

static constexpr uint8_t kMCP23008DefaultAddr = 0x20; // Default I2C address for MCP23008

enum McpRegisterMap : uint8_t {
  kIoDirectionReg = 0x00,       //<! I/O Direction Register
  kInputPolarityReg = 0x01,     //<! Input Polarity Register
  kInterruptOnChangeReg = 0x02, //<! Interrupt-on-Change Control Register
  kCompareValueReg = 0x03,      //<! Default Compare Register for Interrupt-on-Change
  kInterruptControlReg = 0x04,  //<! Interrupt Control Register
  kConfigurationReg = 0x05,     //<! I/O Expander Configuration Register
  kPullupReg = 0x06,            //<! GPIO Pull-Up Resistor Register
  kInterruptFlagReg = 0x07,     //<! Interrupt Flag
  kInterruptCaptureReg = 0x08,  //<! Interrupt Captured Value for Port Register (Read-only)
  kGPIOReg = 0x09,              //<! General Purpose IO
  kOutputLatchReg = 0x0A,       //<! Output Latch Register

};

struct McpConfig {
  uint8_t unused : 2;
  uint8_t sequential_operation : 1; //<! SEQOP: Sequential Operation Mode
  uint8_t slew_rate : 1;            //<! DISSLW: Slew Rate Control Bit for SDA Output
  uint8_t unused2 : 1;              //<! unused on i2c
  uint8_t open_drain : 1;           //<! ODR: This bit configures the INT pin as an open-drain output
  uint8_t interrupt_polarity : 1;   //<! INTPOL: This bit sets the polarity of the INT output pin
  uint8_t unused3 : 1;
};

class MCP23008 {
private:
  static constexpr uint8_t kI2CAddress = kMCP23008DefaultAddr;

  i2c_master_dev_handle_t _dev_handle;
  uint8_t _i2c_addr;
  std::shared_ptr<I2cManager> _i2c_mgr;
  gpio_num_t _interrupt_pin;

public:
  explicit MCP23008();
  esp_err_t Init();

  esp_err_t Read(uint8_t reg, uint8_t &value);
  esp_err_t ReadBit(uint8_t reg, bool &value, uint8_t bit);
  esp_err_t Write(uint8_t reg, uint8_t value);
  esp_err_t WriteBit(uint8_t reg, const bool value, uint8_t bit);

  bool GpioGetLevel(uint8_t number);
  esp_err_t GpioSetLevel(uint8_t number, bool level);
  esp_err_t GpioToggle(uint8_t number);
  esp_err_t GpioGetDirection(uint8_t number, gpio_mode_t &mode);
  esp_err_t GpioSetDirection(uint8_t number, gpio_mode_t const mode);
  esp_err_t GpioGetPullup(uint8_t number, bool &enable);
  esp_err_t GpioSetPullup(uint8_t number, const bool enable);

  // TODO: Interrupt stuff
  // esp_err_t GetInterruptOnChange(uint8_t number, bool &enable);
  // esp_err_t SetInterruptOnChange(uint8_t number, const bool enable);
};