#include "i2c_manager.hpp"

#include <driver/i2c_master.h>
#include <esp_err.h>
#include <funlog.h>

#include <bitset>
#include <mutex>
#include <string>

#include "iom_config.h"

esp_err_t I2cManager::Init() {
  std::lock_guard<std::mutex> lock(_mutex);
  if (_initialized) {
    return ESP_OK;
  }
  gpio_num_t pin_scl, pin_sda;
#if defined(CONFIG_IOM_EXTERNAL_ENABLE)
  if (_external) {
    pin_scl = (gpio_num_t)CONFIG_IOM_EXTERNAL_I2C_SCL_PIN;
    pin_sda = (gpio_num_t)CONFIG_IOM_EXTERNAL_I2C_SDA_PIN;
  } else {
#endif
    pin_scl = (gpio_num_t)CONFIG_IOM_I2C_SCL_PIN;
    pin_sda = (gpio_num_t)CONFIG_IOM_I2C_SDA_PIN;
#if defined(CONFIG_IOM_EXTERNAL_ENABLE)
  }
#endif
  i2c_master_bus_config_t bus_config = {
      .i2c_port = -1,
      .sda_io_num = pin_sda,
      .scl_io_num = pin_scl,
      .clk_source = I2C_CLK_SRC_DEFAULT,
      .flags =
          {
              .enable_internal_pullup = true,
          },
  };

  FLOG_INFO("Initializing I2C on SCL pin %d, SDA pin %d", pin_scl, pin_sda);

  esp_err_t err = i2c_new_master_bus(&bus_config, &_bus_handle);

  if (err != ESP_OK) {
    FLOG_ERROR("Failed to initialize master bus: %s", esp_err_to_name(err));
    return err;
  }

  _initialized = true;
  return ESP_OK;
}

esp_err_t I2cManager::Cleanup() {
  std::lock_guard<std::mutex> lock(_mutex);
  if (!_initialized) {
    return ESP_OK;
  }
  FLOG_INFO("Cleaning up I2C manager");

  i2c_del_master_bus(_bus_handle);
  _initialized = false;
  return ESP_OK;
}

esp_err_t I2cManager::AddDevice(i2c_device_config_t* config, i2c_master_dev_handle_t* dev_handle) {
  std::lock_guard<std::mutex> lock(_mutex);
  if (!_initialized) {
    return ESP_ERR_INVALID_STATE;
  }

  return i2c_master_bus_add_device(_bus_handle, config, dev_handle);
}

esp_err_t I2cManager::ReadRegister(i2c_master_dev_handle_t dev_handle, uint8_t reg, uint8_t* data, size_t len) {
  std::lock_guard<std::mutex> lock(_mutex);
  if (!_initialized) {
    return ESP_ERR_INVALID_STATE;
  }
  return i2c_master_transmit_receive(dev_handle, &reg, 1, data, len, kTimeoutMs);
}

esp_err_t I2cManager::Read(i2c_master_dev_handle_t dev_handle, uint8_t* data, size_t len) {
  std::lock_guard<std::mutex> lock(_mutex);
  if (!_initialized) {
    return ESP_ERR_INVALID_STATE;
  }
  return i2c_master_receive(dev_handle, data, len, kTimeoutMs);
}

esp_err_t I2cManager::WriteRegister(i2c_master_dev_handle_t dev_handle, uint8_t reg, uint8_t* data, size_t len) {
  std::lock_guard<std::mutex> lock(_mutex);
  if (!_initialized) return ESP_ERR_INVALID_STATE;

  // TODO: is this needed?
  uint8_t* buf = (uint8_t*)malloc(len + 1);
  if (!buf) return ESP_ERR_NO_MEM;

  buf[0] = reg;
  memcpy(buf + 1, data, len);

  esp_err_t err = i2c_master_transmit(dev_handle, buf, len + 1, kTimeoutMs);
  free(buf);
  return err;
}

esp_err_t I2cManager::Write(i2c_master_dev_handle_t dev_handle, const uint8_t* data, size_t len) {
  std::lock_guard<std::mutex> lock(_mutex);
  if (!_initialized) {
    return ESP_ERR_INVALID_STATE;
  }
  return i2c_master_transmit(dev_handle, data, len, kTimeoutMs);
}

esp_err_t I2cManager::Scan() {
  std::lock_guard<std::mutex> lock(_mutex);
  if (!_initialized) {
    return ESP_ERR_INVALID_STATE;
  }
  uint8_t slave = 0x00;
  esp_err_t err;
  _device_addresses.clear();
  for (int i = 0; i <= 127; i++) {
    err = i2c_master_probe(_bus_handle, slave, -1);
    if (err == ESP_OK) {
      FLOG_DEBUG("slave device address found on 0x%X\n", slave);
      _device_addresses.push_back(slave);
    }
    usleep(10000);  // Small delay to avoid bus overload
    slave = slave + 1;
  }
  return ESP_OK;
}

esp_err_t I2cManager::GetScannedAddresses(std::vector<uint8_t>& addresses) const {
  std::lock_guard<std::mutex> lock(_mutex);
  if (!_initialized) {
    return ESP_ERR_INVALID_STATE;
  }
  addresses = _device_addresses;
  return ESP_OK;
}

esp_err_t I2cManager::Probe(const uint16_t address) {
  esp_err_t ret = i2c_master_probe(_bus_handle, address, kTimeoutMs);
  if (ret != ESP_OK) {
    FLOG_DEBUG("No device found at address 0x%X: %s", address, esp_err_to_name(ret));
  }
  return ret;
}
