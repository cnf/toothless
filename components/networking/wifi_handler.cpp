#include "wifi_handler.hpp"

#include <esp_netif.h>

#include <cstring>

#include "funlog.h"
#include "networking.hpp"

extern "C" {
#include <pubsub.h>
}

namespace networking {

WiFiHandler::WiFiHandler() : _state(WiFiState::kDisconnected) {}

WiFiHandler::~WiFiHandler() { Deinit(); }

bool WiFiHandler::Init() {
  if (_initialized) {
    FLOG_WARN("WiFi already initialized");
    return true;
  }

  FLOG_INFO("Initializing WiFi");

  // Initialize TCP/IP stack
  ESP_ERROR_CHECK(esp_netif_init());

  // Create default event loop if not exists
  esp_err_t err = esp_event_loop_create_default();
  if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
    FLOG_ERROR("Failed to create event loop: %s", esp_err_to_name(err));
    return false;
  }

  // Create default WiFi station
  esp_netif_create_default_wifi_sta();

  // Initialize WiFi with default config
  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  err = esp_wifi_init(&cfg);
  if (err != ESP_OK) {
    FLOG_ERROR("Failed to init WiFi: %s", esp_err_to_name(err));
    return false;
  }

  // Register event handlers
  err = esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &WiFiHandler::EventHandler, this,
                                            &_wifi_event_handle);
  if (err != ESP_OK) {
    FLOG_ERROR("Failed to register WiFi event handler");
    return false;
  }

  err = esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &WiFiHandler::EventHandler, this,
                                            &_ip_event_handle);
  if (err != ESP_OK) {
    FLOG_ERROR("Failed to register IP event handler");
    return false;
  }

  err = esp_wifi_set_mode(WIFI_MODE_STA);
  if (err != ESP_OK) {
    FLOG_ERROR("Failed to set WiFi mode: %s", esp_err_to_name(err));
    return false;
  }

  _initialized = true;
  FLOG_INFO("WiFi initialized");
  return true;
}

void WiFiHandler::Deinit() {
  if (!_initialized) return;

  FLOG_INFO("Deinitializing WiFi");

  Disconnect();

  if (_wifi_event_handle) {
    esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, _wifi_event_handle);
    _wifi_event_handle = nullptr;
  }
  if (_ip_event_handle) {
    esp_event_handler_instance_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, _ip_event_handle);
    _ip_event_handle = nullptr;
  }

  esp_wifi_deinit();

  esp_event_loop_delete_default();

  delete _config;
  _config = nullptr;
  _initialized = false;
}

bool WiFiHandler::Connect(const WiFiConfig& config) {
  if (!_initialized) {
    FLOG_ERROR("WiFi not initialized");
    return false;
  }

  FLOG_INFO("Connecting to %s", config.ssid.c_str());

  // Store config
  delete _config;
  _config = new WiFiConfig(config);
  _retry_count = 0;

  // Configure WiFi
  wifi_config_t wifi_cfg = {};
  std::strncpy(reinterpret_cast<char*>(wifi_cfg.sta.ssid), config.ssid.c_str(), sizeof(wifi_cfg.sta.ssid) - 1);
  std::strncpy(reinterpret_cast<char*>(wifi_cfg.sta.password), config.password.c_str(),
               sizeof(wifi_cfg.sta.password) - 1);
  wifi_cfg.sta.threshold.authmode = config.password.empty() ? WIFI_AUTH_OPEN : WIFI_AUTH_WPA2_PSK;

  esp_err_t err = esp_wifi_set_config(WIFI_IF_STA, &wifi_cfg);
  if (err != ESP_OK) {
    FLOG_ERROR("Failed to set WiFi config: %s", esp_err_to_name(err));
    return false;
  }

  err = esp_wifi_start();
  if (err != ESP_OK) {
    FLOG_ERROR("Failed to start WiFi: %s", esp_err_to_name(err));
    return false;
  }

  PublishState(WiFiState::kConnecting);
  return true;
}

void WiFiHandler::Disconnect() {
  if (_state == WiFiState::kDisconnected) return;

  FLOG_INFO("Disconnecting WiFi");
  esp_wifi_disconnect();
  esp_wifi_stop();
  PublishState(WiFiState::kDisconnected);
}

WiFiState WiFiHandler::GetState() const { return _state; }

void WiFiHandler::SetStateCallback(StateCallback callback) { _state_callback = std::move(callback); }

std::string WiFiHandler::GetIPAddress() const {
  esp_netif_t* netif = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
  if (!netif) return "";

  esp_netif_ip_info_t ip_info;
  if (esp_netif_get_ip_info(netif, &ip_info) != ESP_OK) return "";

  char ip_str[16];
  snprintf(ip_str, sizeof(ip_str), IPSTR, IP2STR(&ip_info.ip));
  return std::string(ip_str);
}

std::string WiFiHandler::GetSSID() const { return _config ? _config->ssid : ""; }

int8_t WiFiHandler::GetRSSI() const {
  wifi_ap_record_t ap_info;
  if (esp_wifi_sta_get_ap_info(&ap_info) == ESP_OK) {
    return ap_info.rssi;
  }
  return 0;
}

void WiFiHandler::EventHandler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
  auto* handler = static_cast<WiFiHandler*>(arg);
  if (event_base == WIFI_EVENT) {
    handler->HandleWiFiEvent(event_id, event_data);
  } else if (event_base == IP_EVENT) {
    handler->HandleIPEvent(event_id, event_data);
  }
}

void WiFiHandler::HandleWiFiEvent(int32_t event_id, void* event_data) {
  switch (event_id) {
    case WIFI_EVENT_STA_START:
      FLOG_DEBUG("WiFi started, connecting...");
      esp_wifi_connect();
      break;

    case WIFI_EVENT_STA_DISCONNECTED: {
      auto* event = static_cast<wifi_event_sta_disconnected_t*>(event_data);
      FLOG_WARN("Disconnected from AP, reason: %d", event->reason);

      if (_config && _config->auto_reconnect && _retry_count < _config->max_retries) {
        _retry_count++;
        FLOG_INFO("Retrying connection (%d/%d)", _retry_count, _config->max_retries);
        esp_wifi_connect();
      } else {
        PublishState(WiFiState::kError);
      }
      break;
    }

    case WIFI_EVENT_STA_CONNECTED:
      FLOG_INFO("Connected to AP");
      break;

    default:
      break;
  }
}

void WiFiHandler::HandleIPEvent(int32_t event_id, void* event_data) {
  if (event_id == IP_EVENT_STA_GOT_IP) {
    auto* event = static_cast<ip_event_got_ip_t*>(event_data);
    FLOG_INFO("Got IP: " IPSTR, IP2STR(&event->ip_info.ip));
    _retry_count = 0;
    PublishState(WiFiState::kConnected);
  }
}

void WiFiHandler::PublishState(WiFiState state) {
  _state = state;
  if (_state_callback) {
    _state_callback(state);
  }
}

}  // namespace networking
