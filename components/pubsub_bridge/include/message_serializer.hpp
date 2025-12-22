/// @file message_serializer.hpp
/// @brief Serializes/deserializes pubsub messages for transport
#pragma once

#include <esp_err.h>
#include <stddef.h>
#include <stdint.h>

extern "C" {
#include <pubsub.h>
}

namespace toothless {

/// @brief Serializes pubsub messages into binary format for transport
/// Format: [TYPE:1][DATA:n]
/// - INT: [0x01][int64:8]
/// - DBL: [0x02][double:8]
/// - BOOL: [0x03][bool:1]
/// - STR: [0x04][len:2][string:n]
/// - BUF: [0x05][len:2][data:n]
/// - NIL: [0x06]
class MessageSerializer {
 public:
  /// @brief Serialize a pubsub message to binary
  /// @param msg Message to serialize
  /// @param buffer Output buffer
  /// @param max_size Maximum buffer size
  /// @param actual_size Actual serialized size
  /// @return ESP_OK on success
  static esp_err_t Serialize(const ps_msg_t* msg, uint8_t* buffer, size_t max_size, size_t* actual_size);

  /// @brief Deserialize binary data and publish to local bus
  /// @param topic Topic to publish to
  /// @param buffer Serialized data
  /// @param size Size of serialized data
  /// @return ESP_OK on success
  static esp_err_t Deserialize(const char* topic, const uint8_t* buffer, size_t size);

 private:
  enum class SerializedType : uint8_t {
    TYPE_INT = 0x01,
    TYPE_DBL = 0x02,
    TYPE_BOOL = 0x03,
    TYPE_STR = 0x04,
    TYPE_BUF = 0x05,
    TYPE_NIL = 0x06,
  };
};

}  // namespace toothless
