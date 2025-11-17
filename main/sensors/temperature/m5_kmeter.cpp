#include "m5_kmeter.hpp"

#include <esp_check.h>

#include "funlog.h"

namespace toothless {
M5KMeter::M5KMeter() {
  _i2c_mgr = I2cManager::GetInstance();
  i2c_device_config_t device_config = {
      .dev_addr_length = I2C_ADDR_BIT_LEN_7,
      .device_address = kMeterDefaultAddr,
      .scl_speed_hz = I2cManager::kClockSpeedHz,
  };
  ESP_ERROR_CHECK_WITHOUT_ABORT(_i2c_mgr->AddDevice(&device_config, &_dev_handle));
}

M5KMeter::~M5KMeter() {
  if (_dev_handle != nullptr) {
    // _i2c_mgr->RemoveDevice(_dev_handle);
    _dev_handle = nullptr;
  }
}

esp_err_t M5KMeter::ReadCelsius(uint32_t& celsius) {
  int32_t res = 0;

  ESP_RETURN_ON_ERROR(_i2c_mgr->ReadRegister(_dev_handle, kMeterRegTempertureValue, (uint8_t*)&res, 4),
                      FLOG_SHORT_FILENAME, "Failed to read temperature value from M5 KMeter");
  // readBytes(_addr, kMeterRegTempertureValue, (uint8_t*)&res, 4);
  // FLOG_INFO("M5 KMeter raw temperature value: %li", res);
  celsius = res;
  return ESP_OK;
}

}  // namespace toothless