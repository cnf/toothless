// cSpell: words fram
#include "fram.hpp"

#include <driver/i2c_master.h>

#include <cstring>
#include <vector>

#include "funlog.h"
#include "i2c_manager.hpp"

FRAM::FRAM() : _i2c_mgr(I2cManager::GetInstance()) {}
FRAM::~FRAM() = default;

esp_err_t FRAM::Init() {
  FLOG_INFO("Setting up F-RAM");
  _i2c_addr = kCY15B064JDefaultAddr;
  _addr_wordlen = 2;  // CY15B064J has 2-byte memory addresses

  i2c_device_config_t dev_cfg = {
      .dev_addr_length = I2C_ADDR_BIT_LEN_7,
      .device_address = _i2c_addr,
      .scl_speed_hz = I2cManager::kClockSpeedHz,
  };

  esp_err_t err = _i2c_mgr->AddDevice(&dev_cfg, &_dev_handle);
  if (err != ESP_OK) {
    FLOG_ERROR("PWR: %s", esp_err_to_name(err));
    return err;
  }

  const char test[] = "Hello FRAM";
  err = Write(0x0000, (const uint8_t*)test, sizeof(test) - 1);
  if (err != ESP_OK) {
    FLOG_ERROR("FRAM write fail %s", esp_err_to_name(err));
    return err;
  }

  uint8_t buffer[10] = {0};
  err = Read(0x0000, buffer, sizeof(buffer));
  if (err != ESP_OK) {
    FLOG_ERROR("FRAM read fail %s", esp_err_to_name(err));
    return err;
  }
  FLOG_INFO("FRAM Read: %.*s", (int)sizeof(buffer), buffer);
  return ESP_OK;
}

esp_err_t FRAM::Read(uint16_t memory_address, uint8_t* data, uint16_t size) {
  if (data == nullptr) {
    return ESP_ERR_INVALID_ARG;
  }
  std::vector<uint8_t> addr_buf(_addr_wordlen);
  uint16_t be_addr = NetworkByteSwap<uint16_t>(memory_address);
  std::memcpy(addr_buf.data(), &be_addr, _addr_wordlen);

  return i2c_master_transmit_receive(_dev_handle, addr_buf.data(), _addr_wordlen, data, size, 3);
}

esp_err_t FRAM::Write(uint16_t memory_address, const uint8_t* data, uint16_t size) {
  if (data == nullptr) {
    return ESP_ERR_INVALID_ARG;
  }
  std::vector<uint8_t> addr_buf(_addr_wordlen + size);
  uint16_t be_addr = NetworkByteSwap<uint16_t>(memory_address);
  std::memcpy(addr_buf.data(), &be_addr, _addr_wordlen);
  std::memcpy(addr_buf.data() + _addr_wordlen, data, size);
  return i2c_master_transmit(_dev_handle, addr_buf.data(), _addr_wordlen + size, 3);
}
