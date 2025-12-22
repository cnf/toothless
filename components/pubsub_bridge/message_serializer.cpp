/// @file message_serializer.cpp
/// @brief Implementation of message serialization
#include "message_serializer.hpp"

#include <funlog.h>
#include <string.h>

namespace toothless {

esp_err_t MessageSerializer::Serialize(const ps_msg_t* msg, uint8_t* buffer, size_t max_size, size_t* actual_size) {
  if (!msg || !buffer || !actual_size) {
    return ESP_ERR_INVALID_ARG;
  }

  size_t idx = 0;

  if (PS_IS_INT(msg)) {
    if (max_size < 9) return ESP_ERR_NO_MEM;  // 1 type + 8 bytes
    buffer[idx++] = static_cast<uint8_t>(SerializedType::TYPE_INT);
    // Network byte order (big-endian)
    int64_t val = msg->int_val;
    for (int i = 7; i >= 0; i--) {
      buffer[idx++] = (val >> (i * 8)) & 0xFF;
    }
  } else if (PS_IS_DBL(msg)) {
    if (max_size < 9) return ESP_ERR_NO_MEM;
    buffer[idx++] = static_cast<uint8_t>(SerializedType::TYPE_DBL);
    // Serialize double as 8 bytes
    uint64_t val;
    memcpy(&val, &msg->dbl_val, sizeof(double));
    for (int i = 7; i >= 0; i--) {
      buffer[idx++] = (val >> (i * 8)) & 0xFF;
    }
  } else if (PS_IS_BOOL(msg)) {
    if (max_size < 2) return ESP_ERR_NO_MEM;
    buffer[idx++] = static_cast<uint8_t>(SerializedType::TYPE_BOOL);
    buffer[idx++] = msg->bool_val ? 1 : 0;
  } else if (PS_IS_STR(msg)) {
    if (!msg->str_val) {
      // Null string, treat as empty
      if (max_size < 3) return ESP_ERR_NO_MEM;
      buffer[idx++] = static_cast<uint8_t>(SerializedType::TYPE_STR);
      buffer[idx++] = 0;  // len high
      buffer[idx++] = 0;  // len low
    } else {
      size_t str_len = strlen(msg->str_val);
      if (str_len > 65535) return ESP_ERR_INVALID_SIZE;  // Max 16-bit length
      if (max_size < 3 + str_len) return ESP_ERR_NO_MEM;

      buffer[idx++] = static_cast<uint8_t>(SerializedType::TYPE_STR);
      buffer[idx++] = (str_len >> 8) & 0xFF;
      buffer[idx++] = str_len & 0xFF;
      memcpy(&buffer[idx], msg->str_val, str_len);
      idx += str_len;
    }
  } else if (PS_IS_BUF(msg)) {
    size_t buf_len = msg->buf_val.sz;
    if (buf_len > 65535) return ESP_ERR_INVALID_SIZE;
    if (max_size < 3 + buf_len) return ESP_ERR_NO_MEM;

    buffer[idx++] = static_cast<uint8_t>(SerializedType::TYPE_BUF);
    buffer[idx++] = (buf_len >> 8) & 0xFF;
    buffer[idx++] = buf_len & 0xFF;
    if (buf_len > 0 && msg->buf_val.ptr) {
      memcpy(&buffer[idx], msg->buf_val.ptr, buf_len);
      idx += buf_len;
    }
  } else {  // NIL or unknown
    if (max_size < 1) return ESP_ERR_NO_MEM;
    buffer[idx++] = static_cast<uint8_t>(SerializedType::TYPE_NIL);
  }

  *actual_size = idx;
  return ESP_OK;
}

esp_err_t MessageSerializer::Deserialize(const char* topic, const uint8_t* buffer, size_t size) {
  if (!topic || !buffer || size < 1) {
    return ESP_ERR_INVALID_ARG;
  }

  SerializedType type = static_cast<SerializedType>(buffer[0]);
  size_t idx = 1;

  switch (type) {
    case SerializedType::TYPE_INT: {
      if (size < 9) return ESP_ERR_INVALID_SIZE;
      int64_t val = 0;
      for (int i = 0; i < 8; i++) {
        val = (val << 8) | buffer[idx++];
      }
      PS_PUB_INT(topic, val);
      break;
    }

    case SerializedType::TYPE_DBL: {
      if (size < 9) return ESP_ERR_INVALID_SIZE;
      uint64_t raw = 0;
      for (int i = 0; i < 8; i++) {
        raw = (raw << 8) | buffer[idx++];
      }
      double val;
      memcpy(&val, &raw, sizeof(double));
      PS_PUB_DBL(topic, val);
      break;
    }

    case SerializedType::TYPE_BOOL: {
      if (size < 2) return ESP_ERR_INVALID_SIZE;
      bool val = buffer[idx++] != 0;
      PS_PUB_BOOL(topic, val);
      break;
    }

    case SerializedType::TYPE_STR: {
      if (size < 3) return ESP_ERR_INVALID_SIZE;
      uint16_t str_len = (buffer[idx] << 8) | buffer[idx + 1];
      idx += 2;
      if (size < 3 + str_len) return ESP_ERR_INVALID_SIZE;

      // Create null-terminated string
      char* str = new char[str_len + 1];
      if (str_len > 0) {
        memcpy(str, &buffer[idx], str_len);
      }
      str[str_len] = '\0';

      PS_PUB_STR(topic, str);
      delete[] str;
      break;
    }

    case SerializedType::TYPE_BUF: {
      if (size < 3) return ESP_ERR_INVALID_SIZE;
      uint16_t buf_len = (buffer[idx] << 8) | buffer[idx + 1];
      idx += 2;
      if (size < 3 + buf_len) return ESP_ERR_INVALID_SIZE;

      // Allocate and copy buffer
      void* buf = nullptr;
      if (buf_len > 0) {
        buf = malloc(buf_len);
        if (!buf) return ESP_ERR_NO_MEM;
        memcpy(buf, &buffer[idx], buf_len);
      }

      PS_PUB_BUF(topic, buf, buf_len, free);
      break;
    }

    case SerializedType::TYPE_NIL: {
      PS_PUB_NIL(topic);
      break;
    }

    default:
      FLOG_ERROR("Unknown serialized type: 0x%02X", buffer[0]);
      return ESP_ERR_INVALID_ARG;
  }

  return ESP_OK;
}

}  // namespace toothless
