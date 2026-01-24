#pragma once

#include <memory>
#include <string>

#include "config_mgr.hpp"

namespace networking {

struct NetworkingConfig {
  bool enabled = false;
  char ssid[32];
  char password[65];
  char hostname[32];
};

inline ConfigEntries config_entries = {
    ConfigEntry("enabled", "Enable WiFi connectivity", "", false, ""),
    ConfigEntry("ssid", "Wifi SSID", "len=1:32", std::string("Toothless"), ""),
    ConfigEntry("password", "Wifi Password", "len=8:64,password=1", std::string("password"), ""),
    ConfigEntry("hostname", "Device hostname", "len=1:32", std::string("toothless"), ""),
    // ConfigEntry("theme", "UI theme", themes::MakeFormat(), std::string("toothless"), ""),
    // ConfigEntry("brightness", "Screen brightness (0-100)", "min=0,max=100", 80, "%"),
    // ConfigEntry("portrait", "Toggle portrait display mode", "", false, ""),
    // ConfigEntry("dark_mode", "Toggle Dark mode theme", "", true, ""),

};

/// Forward declarations for handler classes
class WiFiHandler;
class OTAHandler;
class HttpServer;

/// Connection state for WiFi
enum class WiFiState { kDisconnected, kConnecting, kConnected, kError };

/// OTA update state
enum class OTAState { kIdle, kChecking, kDownloading, kInstalling, kComplete, kError };

/// Configuration for WiFi connection
struct WiFiConfig {
  std::string ssid;
  std::string password;
  bool auto_reconnect = true;
  uint8_t max_retries = 5;
  std::string hostname = "toothless";
};

/// Configuration for OTA updates
struct OTAConfig {
  std::string url;
  bool auto_check = false;
  uint32_t check_interval_ms = 3600000;  // 1 hour default
};

/// @brief Main network manager coordinating WiFi and OTA
/// @details Publishes state changes to pubsub topics for other components.
///          Future: MQTT and web interface can be added as separate handlers.
class NetworkManager {
 public:
  NetworkManager();
  ~NetworkManager();

  /// @brief Initialize networking subsystem
  void Init();

  /// @brief Main loop to be called periodically
  void Loop();

  /// @brief Startup networking subsystem
  /// @return true on success
  bool Startup();

  /// @brief Shutdown networking subsystem
  void Shutdown();

  /// @brief Connect to WiFi with given config
  /// @return true if connection started
  bool Connect();

  /// @brief Disconnect from WiFi
  void Disconnect();

  /// @brief Get current WiFi state
  WiFiState GetWiFiState() const;

  /// @brief Check if connected to WiFi
  bool IsConnected() const;

  /// @brief Start OTA update from configured URL
  /// @param url Firmware URL (uses config URL if empty)
  /// @return true if OTA started
  bool StartOTA(const std::string& url = "");

  /// @brief Configure OTA settings
  void ConfigureOTA(const OTAConfig& config);

  /// @brief Get current OTA state
  OTAState GetOTAState() const;

  /// @brief Start HTTP server for OTA uploads
  /// @param port Server port (default 80)
  /// @return true if server started
  bool StartHttpServer(uint16_t port = 80);

  /// @brief Stop HTTP server
  void StopHttpServer();

  /// @brief Check if HTTP server is running
  bool IsHttpServerRunning() const;

  // Delete copy/move for singleton-like usage
  NetworkManager(const NetworkManager&) = delete;
  NetworkManager& operator=(const NetworkManager&) = delete;

 private:
  std::unique_ptr<WiFiHandler> _wifi;
  std::unique_ptr<OTAHandler> _ota;
  std::unique_ptr<HttpServer> _http;
  ps_subscriber_t* _subscription;
  ConfigEntries* _config_entries;        // UI configuration entries
  std::shared_ptr<SettingsMap> _config;  // UI settings map
  bool _initialized = false;

  esp_err_t ApplySettings();
};

}  // namespace networking