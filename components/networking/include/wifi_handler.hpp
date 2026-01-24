#pragma once

#include <esp_event.h>
#include <esp_wifi.h>

#include <functional>
#include <string>

namespace networking {

enum class WiFiState;
struct WiFiConfig;

/// @brief Handles WiFi connection lifecycle
/// @details Manages ESP-IDF WiFi driver, events, and reconnection logic.
class WiFiHandler {
 public:
  using StateCallback = std::function<void(WiFiState)>;

  WiFiHandler();
  ~WiFiHandler();

  /// @brief Initialize WiFi subsystem
  /// @return true on success
  bool Init();

  /// @brief Deinitialize WiFi subsystem
  void Deinit();

  /// @brief Connect to AP with given config
  /// @param config WiFi configuration
  /// @return true if connection initiated
  bool Connect(const WiFiConfig& config);

  /// @brief Disconnect from current AP
  void Disconnect();

  /// @brief Set the hostname for the device
  /// @param hostname
  void SetHostname(const std::string& hostname);

  /// @brief Enable mDNS with the current hostname
  void EnableMDNS();

  /// @brief Get current connection state
  WiFiState GetState() const;

  /// @brief Set callback for state changes
  void SetStateCallback(StateCallback callback);

  /// @brief Get current IP address as string
  std::string GetIPAddress() const;

  /// @brief Get current SSID
  std::string GetSSID() const;

  /// @brief Get RSSI (signal strength)
  int8_t GetRSSI() const;

 private:
  static void EventHandler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data);
  void HandleWiFiEvent(int32_t event_id, void* event_data);
  void HandleIPEvent(int32_t event_id, void* event_data);
  void PublishState(WiFiState state);

  WiFiState _state;
  WiFiConfig* _config = nullptr;
  StateCallback _state_callback;
  esp_event_handler_instance_t _wifi_event_handle = nullptr;
  esp_event_handler_instance_t _ip_event_handle = nullptr;
  uint8_t _retry_count = 0;
  bool _initialized = false;
};

}  // namespace networking
