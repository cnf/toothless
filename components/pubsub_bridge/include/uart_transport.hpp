/// @file uart_transport.hpp
/// @brief UART transport implementation with framing and CRC
#pragma once

#include <driver/uart.h>

#include "transport.hpp"

namespace toothless {

/// @brief UART configuration for transport
struct UartTransportConfig {
  uart_port_t port = UART_NUM_1;
  int tx_pin = -1;  // Must be set
  int rx_pin = -1;  // Must be set
  int baud_rate = 921600;
  size_t rx_buffer_size = 2048;
  size_t tx_buffer_size = 2048;
};

/// @brief UART transport with STX/ETX framing and CRC16 error detection
class UartTransport : public ITransport {
 public:
  explicit UartTransport(const UartTransportConfig& config);
  ~UartTransport();

  esp_err_t Init() override;
  esp_err_t Send(const char* topic, const void* data, size_t len) override;
  esp_err_t Receive(char* topic, void* data, size_t* len, uint32_t timeout_ms) override;

 private:
  /// @brief Calculate CRC16-CCITT
  static uint16_t Crc16(const uint8_t* data, size_t len);

  /// @brief Receive state machine states
  enum class RxState { WAIT_STX, READ_LENGTH, READ_PAYLOAD, VALIDATE };

  UartTransportConfig _config;
  RxState _rx_state = RxState::WAIT_STX;
  uint16_t _rx_frame_len = 0;
  uint16_t _rx_bytes_read = 0;
  uint8_t _rx_frame_buffer[512];

  static constexpr uint8_t STX = 0x02;
  static constexpr uint8_t ETX = 0x03;
  static constexpr size_t MAX_FRAME_SIZE = 512;
};

}  // namespace toothless
