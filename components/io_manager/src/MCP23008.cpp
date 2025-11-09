#include "config.h"

#include "MCP23008.hpp"
#include "funlog.h"
#include <bitset>

MCP23008::MCP23008() : _i2c_addr(kMCP23008DefaultAddr), _i2c_mgr(I2cManager::GetInternalInstance()) {}

esp_err_t MCP23008::Init() {
  FLOG_INFO("Setting up Port Expander");
  if (_i2c_mgr->Probe(_i2c_addr) != ESP_OK) {
    FLOG_ERROR("No device found at address 0x%02X", _i2c_addr);
    return ESP_ERR_NOT_FOUND;
  }
  i2c_device_config_t dev_cfg = {
      .dev_addr_length = I2C_ADDR_BIT_LEN_7,
      .device_address = _i2c_addr,
      .scl_speed_hz = I2cManager::kClockSpeedHz,
  };

  ESP_ERROR_CHECK(_i2c_mgr->AddDevice(&dev_cfg, &_dev_handle));

  gpio_mode_t mode;
  ESP_ERROR_CHECK(GpioGetDirection(1, mode));
  return ESP_OK;
}

esp_err_t MCP23008::Read(uint8_t reg, uint8_t &value) {
  esp_err_t err = _i2c_mgr->ReadRegister(_dev_handle, reg, &value, 1);
  if (err != ESP_OK) {
    FLOG_ERROR("i2c read reg 0x%02X fail: %s", reg, esp_err_to_name(err));
  }
  return err;
}

esp_err_t MCP23008::ReadBit(uint8_t reg, bool &value, uint8_t bit) {
  uint8_t data;
  ESP_ERROR_CHECK(_i2c_mgr->ReadRegister(_dev_handle, reg, &data, 1));
  value = (data >> bit) & 0x01;
  return ESP_OK;
}

esp_err_t MCP23008::Write(uint8_t reg, uint8_t value) {
  FLOG_DEBUG("MCP write reg=0x%02X value=0x%02X", reg, value);
  ESP_ERROR_CHECK(_i2c_mgr->WriteRegister(_dev_handle, reg, &value));
  return ESP_OK;
}

esp_err_t MCP23008::WriteBit(uint8_t reg, const bool value, uint8_t bit) {
  // validate bit index
  if (bit >= 8) {
    FLOG_ERROR("WriteBit bad bit %u", bit);
    return ESP_ERR_INVALID_ARG;
  }

  uint8_t data = 0;
  uint8_t target_reg = reg;
  // when driving an output, write the OLAT latch so the expander
  // updates its output latch and reads reflect our change.
  if (reg == McpRegisterMap::kGPIOReg) {
    target_reg = McpRegisterMap::kOutputLatchReg;
  }

  ESP_ERROR_CHECK(_i2c_mgr->ReadRegister(_dev_handle, target_reg, &data, 1));

  if (value) {
    data |= (1 << bit);
  } else {
    data &= ~(1 << bit);
  }

  FLOG_DEBUG("MCP writebit reg=0x%02X bit=%u val=%u -> byte=0x%02X", target_reg, (unsigned)bit, (unsigned)value, data);
  ESP_ERROR_CHECK(_i2c_mgr->WriteRegister(_dev_handle, target_reg, &data));

  uint8_t iod = 0, gpio = 0, olat = 0;
  Read(McpRegisterMap::kIoDirectionReg, iod);
  Read(McpRegisterMap::kGPIOReg, gpio);
  Read(McpRegisterMap::kOutputLatchReg, olat);
  std::bitset<8> bolat = olat;
  std::bitset<8> bgpio = gpio;
  std::bitset<8> biod = iod;
  FLOG_INFO("GP%u: IODIR=%02x(%s) GPIO=%02x(%s) OLAT=%02x (%s)", (unsigned)bit, iod, biod.to_string().c_str(), gpio,
            bgpio.to_string().c_str(), olat, bolat.to_string().c_str());

  return ESP_OK;
}

bool MCP23008::GpioGetLevel(uint8_t number) {
  bool gpio_state;
  ReadBit(McpRegisterMap::kGPIOReg, gpio_state, number);
  return gpio_state;
}

esp_err_t MCP23008::GpioSetLevel(uint8_t number, bool level) {
  ESP_ERROR_CHECK(WriteBit(McpRegisterMap::kGPIOReg, level, number));
  return ESP_OK;
}

esp_err_t MCP23008::GpioToggle(uint8_t number) {
  bool gpio_state;
  ESP_ERROR_CHECK(ReadBit(McpRegisterMap::kGPIOReg, gpio_state, number));
  ESP_ERROR_CHECK(WriteBit(McpRegisterMap::kGPIOReg, !gpio_state, number));
  return ESP_OK;
}

esp_err_t MCP23008::GpioGetDirection(uint8_t number, gpio_mode_t &mode) {
  bool get_mode;
  ESP_ERROR_CHECK(ReadBit(McpRegisterMap::kIoDirectionReg, get_mode, number));
  switch (get_mode) {
  case 0:
    mode = GPIO_MODE_OUTPUT;
    break;
  case 1:
    mode = GPIO_MODE_INPUT;
    break;
  default:
    return ESP_ERR_NOT_SUPPORTED;
    break;
  }
  return ESP_OK;
}

esp_err_t MCP23008::GpioSetDirection(uint8_t number, const gpio_mode_t mode) {
  uint8_t set_mode;
  switch (mode) {
  case GPIO_MODE_INPUT:
    set_mode = 1;
    break;
  case GPIO_MODE_OUTPUT:
    set_mode = 0;
    break;
  default:
    return ESP_ERR_NOT_SUPPORTED;
  }
  ESP_ERROR_CHECK(WriteBit(McpRegisterMap::kIoDirectionReg, set_mode, number));
  return ESP_OK;
}

esp_err_t MCP23008::GpioGetPullup(uint8_t number, bool &enable) {
  ESP_ERROR_CHECK(ReadBit(McpRegisterMap::kPullupReg, enable, number));
  return ESP_OK;
}

esp_err_t MCP23008::GpioSetPullup(uint8_t number, const bool enable) {
  ESP_ERROR_CHECK(WriteBit(McpRegisterMap::kPullupReg, enable, number));
  return ESP_OK;
}
