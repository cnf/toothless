// cSpell: words fram
#pragma once
#include "config.h"

#include "i2c_manager.hpp"

static constexpr uint8_t kCY15B064JDefaultAddr = 0x50;

class FRAM {
public:
  FRAM();
  ~FRAM();
  esp_err_t Init();

  esp_err_t Read(uint16_t memory_address, uint8_t *data, uint16_t size);
  esp_err_t Write(uint16_t memory_address, const uint8_t *data, uint16_t size);

private:
  i2c_master_dev_handle_t _dev_handle;
  uint8_t _i2c_addr;
  std::shared_ptr<I2cManager> _i2c_mgr;
  uint8_t _addr_wordlen; /*!< block address wordlen */
};