/// @file pubsub_bridge.cpp
/// @brief Implementation of pubsub bridge
#include "pubsub_bridge.hpp"

#include <funlog.h>
#include <string.h>

#include "message_serializer.hpp"

namespace toothless {

PubsubBridge::PubsubBridge(ITransport* transport) : _transport(transport) {}

PubsubBridge::~PubsubBridge() {
  if (_subscription) {
    ps_unsubscribe_all(_subscription);
    ps_free_subscriber(_subscription);
  }
  if (_receive_buffer) {
    delete[] _receive_buffer;
  }
}

esp_err_t PubsubBridge::Init(const BridgeConfig& config) {
  _config = config;

  // Initialize transport
  esp_err_t err = _transport->Init();
  if (err != ESP_OK) {
    FLOG_ERROR("Transport init failed: %s", esp_err_to_name(err));
    return err;
  }

  // Allocate receive buffer
  _receive_buffer = new uint8_t[_config.max_data_size];

  // Create subscriber for topics to forward
  if (config.forward_topics.empty()) {
    FLOG_WARN("No topics configured to forward");
    return ESP_OK;
  }

  // Create subscription with queue size based on topic count
  size_t queue_size = config.forward_topics.size() * 2;  // 2x for buffering
  _subscription = ps_new_subscriber(queue_size, PS_STRLIST(""));

  // Subscribe to each topic
  for (const char* topic : config.forward_topics) {
    ps_subscribe(_subscription, topic);
    FLOG_INFO("Forwarding topic: %s", topic);
  }

  FLOG_INFO("Pubsub bridge initialized");
  return ESP_OK;
}

void PubsubBridge::Loop() {
  // Forward local messages to remote
  if (_subscription) {
    ps_msg_t* msg = ps_get(_subscription, 0);
    while (msg != nullptr) {
      // Serialize message
      uint8_t serial_buffer[_config.max_data_size];
      size_t serial_len = 0;

      esp_err_t err = MessageSerializer::Serialize(msg, serial_buffer, sizeof(serial_buffer), &serial_len);
      if (err == ESP_OK) {
        err = _transport->Send(msg->topic, serial_buffer, serial_len);
        if (err != ESP_OK) {
          FLOG_VERBOSE("Send failed for topic %s: %s", msg->topic, esp_err_to_name(err));
        }
      } else {
        FLOG_ERROR("Serialize failed for topic %s: %s", msg->topic, esp_err_to_name(err));
      }

      ps_unref_msg(msg);
      msg = ps_get(_subscription, 0);
    }
  }

  // Receive remote messages and publish locally
  char topic[64];
  size_t len = 0;
  esp_err_t err = _transport->Receive(topic, _receive_buffer, &len, _config.receive_timeout_ms);

  if (err == ESP_OK) {
    // Deserialize and republish to local bus
    err = MessageSerializer::Deserialize(topic, _receive_buffer, len);
    if (err == ESP_OK) {
      FLOG_VERBOSE("Bridged message: %s (%d bytes)", topic, len);
    } else {
      FLOG_ERROR("Deserialize failed for topic %s: %s", topic, esp_err_to_name(err));
    }
  } else if (err != ESP_ERR_TIMEOUT) {
    FLOG_VERBOSE("Receive error: %s", esp_err_to_name(err));
  }
}

}  // namespace toothless
