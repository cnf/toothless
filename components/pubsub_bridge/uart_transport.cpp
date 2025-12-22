/// @file uart_transport.cpp
/// @brief UART transport implementation with robust framing
#include "uart_transport.hpp"

#include <funlog.h>
#include <string.h>

namespace toothless {

UartTransport::UartTransport(const UartTransportConfig& config) : _config(config) {}

UartTransport::~UartTransport() { uart_driver_delete(_config.port); }

esp_err_t UartTransport::Init() {
  if (_config.tx_pin < 0 || _config.rx_pin < 0) {
    FLOG_ERROR("UART pins not configured");
    return ESP_ERR_INVALID_ARG;
  }

  uart_config_t uart_config = {
      .baud_rate = _config.baud_rate,
      .data_bits = UART_DATA_8_BITS,
      .parity = UART_PARITY_DISABLE,
      .stop_bits = UART_STOP_BITS_1,
      .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
      .rx_flow_ctrl_thresh = 122,
      .source_clk = UART_SCLK_DEFAULT,
  };

  esp_err_t err = uart_driver_install(_config.port, _config.rx_buffer_size, _config.tx_buffer_size, 0, nullptr, 0);
  if (err != ESP_OK) {
    FLOG_ERROR("UART driver install failed: %s", esp_err_to_name(err));
    return err;
  }

  err = uart_param_config(_config.port, &uart_config);
  if (err != ESP_OK) {
    FLOG_ERROR("UART param config failed: %s", esp_err_to_name(err));
    return err;
  }

  err = uart_set_pin(_config.port, _config.tx_pin, _config.rx_pin, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
  if (err != ESP_OK) {
    FLOG_ERROR("UART set pin failed: %s", esp_err_to_name(err));
    return err;
  }

  FLOG_INFO("UART transport initialized: port=%d, baud=%d, tx=%d, rx=%d", _config.port, _config.baud_rate,
            _config.tx_pin, _config.rx_pin);

  return ESP_OK;
}

esp_err_t UartTransport::Send(const char* topic, const void* data, size_t len) {
  // Frame format: [STX:1][LEN:2][TOPIC:n][NULL:1][DATA:n][CRC16:2][ETX:1]
  size_t topic_len = strlen(topic);
  size_t payload_size = topic_len + 1 + len;  // topic + null + data
  size_t total_frame_size = 1 + 2 + payload_size + 2 + 1;

  if (total_frame_size > MAX_FRAME_SIZE) {
    FLOG_ERROR("Frame too large: %d bytes", total_frame_size);
    return ESP_ERR_INVALID_SIZE;
  }

  uint8_t frame[MAX_FRAME_SIZE];
  size_t idx = 0;

  // Header
  frame[idx++] = STX;
  frame[idx++] = (payload_size >> 8) & 0xFF;
  frame[idx++] = payload_size & 0xFF;

  // Payload: topic + null + data
  memcpy(&frame[idx], topic, topic_len);
  idx += topic_len;
  frame[idx++] = 0x00;
  memcpy(&frame[idx], data, len);
  idx += len;

  // CRC over payload
  uint16_t crc = Crc16(&frame[3], payload_size);
  frame[idx++] = (crc >> 8) & 0xFF;
  frame[idx++] = crc & 0xFF;

  // Trailer
  frame[idx++] = ETX;

  // Send frame
  int written = uart_write_bytes(_config.port, frame, idx);
  if (written != idx) {
    FLOG_ERROR("UART write incomplete: %d/%d", written, idx);
    return ESP_ERR_INVALID_RESPONSE;
  }

  return ESP_OK;
}

esp_err_t UartTransport::Receive(char* topic, void* data, size_t* len, uint32_t timeout_ms) {
  TickType_t start_ticks = xTaskGetTickCount();
  TickType_t timeout_ticks = pdMS_TO_TICKS(timeout_ms);

  while ((xTaskGetTickCount() - start_ticks) < timeout_ticks) {
    uint8_t byte;
    int read_len = uart_read_bytes(_config.port, &byte, 1, 1);  // 1ms timeout per byte

    if (read_len <= 0) {
      continue;
    }

    switch (_rx_state) {
      case RxState::WAIT_STX:
        if (byte == STX) {
          _rx_state = RxState::READ_LENGTH;
          _rx_bytes_read = 0;
        }
        break;

      case RxState::READ_LENGTH:
        _rx_frame_buffer[_rx_bytes_read++] = byte;
        if (_rx_bytes_read == 2) {
          _rx_frame_len = (_rx_frame_buffer[0] << 8) | _rx_frame_buffer[1];
          if (_rx_frame_len > MAX_FRAME_SIZE - 6) {  // Sanity check
            FLOG_WARN("Invalid frame length: %d", _rx_frame_len);
            _rx_state = RxState::WAIT_STX;
            break;
          }
          _rx_bytes_read = 0;
          _rx_state = RxState::READ_PAYLOAD;
        }
        break;

      case RxState::READ_PAYLOAD:
        _rx_frame_buffer[_rx_bytes_read++] = byte;
        if (_rx_bytes_read == _rx_frame_len + 2) {  // +2 for CRC
          _rx_state = RxState::VALIDATE;
        }
        break;

      case RxState::VALIDATE:
        if (byte != ETX) {
          FLOG_WARN("Missing ETX, got 0x%02X", byte);
          _rx_state = RxState::WAIT_STX;
          break;
        }

        // Validate CRC
        uint16_t received_crc = (_rx_frame_buffer[_rx_frame_len] << 8) | _rx_frame_buffer[_rx_frame_len + 1];
        uint16_t calculated_crc = Crc16(_rx_frame_buffer, _rx_frame_len);

        if (received_crc != calculated_crc) {
          FLOG_WARN("CRC mismatch: expected 0x%04X, got 0x%04X", calculated_crc, received_crc);
          _rx_state = RxState::WAIT_STX;
          break;
        }

        // // Parse payload: topic + null + data
        // size_t topic_len = strnlen((char*)_rx_frame_buffer, _rx_frame_len);
        // if (topic_len >= _rx_frame_len) {
        //   FLOG_WARN("No null terminator in topic");
        //   _rx_state = RxState::WAIT_STX;
        //   break;
        // }

        // Parse payload: topic + null + data
        size_t topic_len = 0;
        while (topic_len < _rx_frame_len && _rx_frame_buffer[topic_len] != 0) {
          topic_len++;
        }
        if (topic_len >= _rx_frame_len) {
          FLOG_WARN("No null terminator in topic");
          _rx_state = RxState::WAIT_STX;
          break;
        }

        memcpy(topic, _rx_frame_buffer, topic_len);
        topic[topic_len] = '\0';

        size_t data_len = _rx_frame_len - topic_len - 1;
        memcpy(data, &_rx_frame_buffer[topic_len + 1], data_len);
        *len = data_len;

        _rx_state = RxState::WAIT_STX;
        return ESP_OK;
    }
  }

  return ESP_ERR_TIMEOUT;
}

uint16_t UartTransport::Crc16(const uint8_t* data, size_t len) {
  uint16_t crc = 0xFFFF;  // CRC16-CCITT initial value

  for (size_t i = 0; i < len; i++) {
    crc ^= (uint16_t)data[i] << 8;
    for (uint8_t j = 0; j < 8; j++) {
      if (crc & 0x8000) {
        crc = (crc << 1) ^ 0x1021;  // CRC16-CCITT polynomial
      } else {
        crc = crc << 1;
      }
    }
  }

  return crc;
}

}  // namespace toothless
