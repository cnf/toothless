/// @file pubsub_bridge.hpp
/// @brief Bridge local pubsub messages to remote board via transport layer
#pragma once

#include <esp_err.h>

#include <vector>

#include "transport.hpp"

extern "C" {
#include <pubsub.h>
}

namespace toothless {

/// @brief Configuration for pubsub bridge
struct BridgeConfig {
  /// @brief Topics to subscribe to and forward (wildcards supported)
  std::vector<const char*> forward_topics;

  /// @brief Maximum message data size (default 256 bytes)
  size_t max_data_size = 256;

  /// @brief Receive timeout in milliseconds
  uint32_t receive_timeout_ms = 10;
};

/// @brief Bridges pubsub messages between local and remote boards
class PubsubBridge {
 public:
  /// @brief Construct bridge with transport layer
  /// @param transport Transport implementation (UART, CAN, etc)
  explicit PubsubBridge(ITransport* transport);

  ~PubsubBridge();

  /// @brief Initialize the bridge with configuration
  /// @param config Bridge configuration
  /// @return ESP_OK on success
  esp_err_t Init(const BridgeConfig& config);

  /// @brief Process bridge messages (call in loop)
  /// Forwards local messages to remote and publishes remote messages locally
  void Loop();

 private:
  ITransport* _transport;
  ps_subscriber_t* _subscription = nullptr;
  BridgeConfig _config;
  uint8_t* _receive_buffer = nullptr;
};

}  // namespace toothless
