/**
 * @file i2c_manager.hpp
 * @brief I2C bus singleton manager for ESP32 using ESP-IDF.
 *
 * This header defines the I2cManager class, which provides a thread-safe singleton interface
 * for managing I2C master bus operations, including device registration, read/write operations,
 * bus scanning, and probing. It supports both external and optional internal I2C buses.
 *
 * Usage:
 *   - Initialize once in main application code using I2cManager::GetInstance().Init().
 *   - Access the singleton instance via I2cManager::GetInstance() or I2cManager::GetInternalInstance().
 *   - Add devices with AddDevice() and use the returned device handle for further operations.
 *
 * Features:
 *   - Thread-safe access using std::mutex.
 *   - Device registration and handle management.
 *   - Register and raw read/write operations.
 *   - Bus scanning and address probing.
 *   - Endian conversion utilities for network and SMBus byte order.
 *
 * @todo Handle both internal and external I2C buses.
 *
 * @author
 * @date
 */
#pragma once

#include <driver/i2c_master.h>
#include <esp_err.h>
#include <freertos/FreeRTOS.h>

#include <bit>
#include <memory>
#include <mutex>
#include <vector>

#include "sdkconfig.h"

/// @brief Convert to/from network byte order (big-endian), depending on host endian-ness
/// @tparam T data type (e.g., uint16_t, uint32_t, float)
/// @param x value to convert
/// @return T in correct byte order
template <typename T>
T NetworkByteSwap(T x) {
  if constexpr (std::endian::native == std::endian::big) {
    return x;
  } else {
    return std::byteswap(x);
  }
}

/// @brief Convert to/from SMBus byte order (little-endian), depending on host endian-ness
/// @tparam T data type (e.g., uint16_t, uint32_t,
/// @param x value to convert
/// @return T in correct byte order
template <typename T>
T SMBusByteSwap(T x) {
  if constexpr (std::endian::native == std::endian::little) {
    return x;
  } else {
    return std::byteswap(x);
  }
}

/*! @file i2c_manager.hpp
  @brief I2C bus singleton manager
  @todo handle internal and external I2C Bus
*/

/// @class I2cManager
/// @brief I2C bus singleton manager
///
/// - Initialize once with @ref I2cManager::Init() in main application code before using in components.
///
/// - Use @ref I2cManager::GetInstance() to get the singleton instance.
///
/// - Add devices with @ref I2cManager::AddDevice() and use the returned device handle for read/write operations.
/// ```cpp
/// // Init once in main
/// I2cManager::GetInstance().Init();
/// // Use in components
/// auto &i2c = I2cManager::GetInstance();
/// i2c_master_dev_handle_t dev;
/// i2c.AddDevice(0x3C, &dev);
/// ```
class I2cManager {
 private:
  i2c_master_bus_handle_t _bus_handle = nullptr;
  // i2c_master_bus_handle_t _int_bus_handle = nullptr; //<! Optional internal bus handle
  mutable std::mutex _mutex;
  bool _initialized = false;
  bool _external = false;                  //<! Whether this instance is for internal I2C bus
  std::vector<uint8_t> _device_addresses;  //<! List of scanned device addresses

  I2cManager(bool internal = false) : _external(internal) {};
  // Delete copy/move operations
  I2cManager(const I2cManager&) = delete;
  I2cManager& operator=(const I2cManager&) = delete;

 public:
  static constexpr uint32_t kClockSpeedHz = 400 * 1000;  //<! Default I2C clock speed
  static constexpr uint32_t kTimeoutMs = 150;            //<! Default I2C timeout
  /// @brief Get singleton instance. It wil be created on first call.
  /// @return Reference to singleton instance
  /// Get shared instance for main I2C bus
  static std::shared_ptr<I2cManager> GetInstance() {
    static std::shared_ptr<I2cManager> instance(new I2cManager(false));
    return instance;
  }

#if defined(CONFIG_IOM_EXTERNAL_ENABLE)

  /// Get shared instance for external I2C bus
  static std::shared_ptr<I2cManager> GetExternalInstance() {
    static std::shared_ptr<I2cManager> ext_instance(new I2cManager(true));
    return ext_instance;
  }
#else
  /// Get shared instance for internal I2C bus
  static std::shared_ptr<I2cManager> GetExternalInstance() { return GetInstance(); }
#endif

  /// @brief Initialize I2C bus
  esp_err_t Init();
  // esp_err_t Init(bool internal = false);

  /// @brief Cleanup I2C bus
  esp_err_t Cleanup();

  /// @brief Add a device to the I2C Master bus, and sets the @ref device_handle
  /// @param config Pointer to filled out device configuration structure
  /// @param dev_handle Pointer to device handle to be filled
  /// @return @ref ESP_OK on success, error code otherwise
  ///
  /// Device handle is filled on success
  /// @note Adding the same device again will result in duplicate entries!
  esp_err_t AddDevice(i2c_device_config_t* config, i2c_master_dev_handle_t* dev_handle);

  /// @brief Check if I2C bus is initialized
  /// @return true if initialized, false otherwise
  bool IsInitialized() const { return _initialized; }

  /// @brief Get the I2C Master bus handle
  /// @return I2C Master bus handle
  i2c_master_bus_handle_t GetBusHandle() const { return _bus_handle; }

  /// @brief Read from device register
  /// @param dev_handle Device handle gotten from @ref I2cManager::AddDevice()
  /// @param reg Register address to read from
  /// @param data Pointer to buffer to store read data
  /// @param len Length of data to read
  /// @return @ref ESP_OK on success, error code otherwise
  esp_err_t ReadRegister(i2c_master_dev_handle_t dev_handle, uint8_t reg, uint8_t* data, size_t len);

  /// @brief Read raw data from device
  /// @param dev_handle Device handle gotten from @ref I2cManager::AddDevice()
  /// @param data Pointer to buffer to store read data
  /// @param len Length of data to read
  /// @return @ref ESP_OK on success, error code otherwise
  esp_err_t Read(i2c_master_dev_handle_t dev_handle, uint8_t* data, size_t len);

  /// @brief Write to device register
  /// @param dev_handle Device handle gotten from @ref I2cManager::AddDevice()
  /// @param reg Register address to write to
  /// @param data Pointer to buffer containing data to write
  /// @return @ref ESP_OK on success, error code otherwise
  esp_err_t WriteRegister(i2c_master_dev_handle_t dev_handle, uint8_t reg, uint8_t* data, size_t len = 2);

  /// @brief Write raw data to device
  /// @param dev_handle Device handle gotten from @ref I2cManager::AddDevice()
  /// @param data Pointer to buffer containing data to write
  /// @param len Length of data to write
  /// @return @ref ESP_OK on success, error code otherwise
  esp_err_t Write(i2c_master_dev_handle_t dev_handle, const uint8_t* data, size_t len);

  /// @brief Transmit data to a device and then receive data from it
  /// @param dev_handle Device handle gotten from @ref I2cManager::AddDevice()
  /// @param tx_data Pointer to buffer containing data to transmit
  /// @param tx_len Length of data to transmit
  /// @param rx_data  Pointer to buffer to store received data
  /// @param rx_len Length of data to receive
  /// @return @ref ESP_OK on success, error code otherwise
  esp_err_t TransmitReceive(i2c_master_dev_handle_t dev_handle, const uint8_t* tx_data, size_t tx_len, uint8_t* rx_data,
                            size_t rx_len);

  /// @brief Scan the I2C bus for all clients
  /// @return @ref ESP_OK on success, error code otherwise
  esp_err_t Scan();

  /// @brief Get the list of scanned device addresses
  /// @param addresses Vector to fill with 7-bit I2C addresses found during scan
  /// @return @ref ESP_OK on success, error code otherwise
  esp_err_t GetScannedAddresses(std::vector<uint8_t>& addresses) const;

  /// @brief  Probe an address to see if it is responding
  /// @param address 7-bit I2C address to probe
  /// @return @ref ESP_OK if device is present, error code otherwise
  esp_err_t Probe(const uint16_t address);
};