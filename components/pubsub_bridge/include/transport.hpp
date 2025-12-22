/// @file transport.hpp
/// @brief Abstract transport interface for pubsub bridge
#pragma once

#include <esp_err.h>
#include <stddef.h>
#include <stdint.h>

namespace toothless {

/// @brief Abstract transport layer for inter-board communication
class ITransport {
 public:
  virtual ~ITransport() = default;

  /// @brief Initialize the transport hardware
  virtual esp_err_t Init() = 0;

  /// @brief Send a pubsub message over the transport
  /// @param topic Topic string (null-terminated)
  /// @param data Message data payload
  /// @param len Length of data payload
  /// @return ESP_OK on success
  virtual esp_err_t Send(const char* topic, const void* data, size_t len) = 0;

  /// @brief Receive a pubsub message from the transport
  /// @param topic Buffer to store received topic (must be >= 64 bytes)
  /// @param data Buffer to store received data
  /// @param len Pointer to store received data length
  /// @param timeout_ms Timeout in milliseconds
  /// @return ESP_OK on success, ESP_ERR_TIMEOUT if no message
  virtual esp_err_t Receive(char* topic, void* data, size_t* len, uint32_t timeout_ms) = 0;
};

}  // namespace toothless
