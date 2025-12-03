#pragma once

#include <esp_http_server.h>

#include <functional>
#include <string>

namespace networking {

/// @brief HTTP server state
enum class HttpServerState { kStopped, kRunning, kError };

/// @brief Configuration for HTTP server
struct HttpServerConfig {
  uint16_t port = 80;
  uint16_t max_uri_handlers = 16;
  size_t stack_size = 8192;
};

/// @brief Handles HTTP server for OTA uploads and future web UI
/// @details Provides endpoints for firmware upload and system status.
///          Extensible for settings API and web interface.
class HttpServer {
 public:
  using OTAProgressCallback = std::function<void(size_t written, size_t total)>;
  using OTACompleteCallback = std::function<void(bool success)>;

  HttpServer();
  ~HttpServer();

  /// @brief Initialize and start HTTP server
  /// @param config Server configuration
  /// @return true on success
  bool Start(const HttpServerConfig& config = {});

  /// @brief Stop HTTP server
  void Stop();

  /// @brief Check if server is running
  bool IsRunning() const;

  /// @brief Get server state
  HttpServerState GetState() const;

  /// @brief Set callback for OTA progress updates
  void SetOTAProgressCallback(OTAProgressCallback callback);

  /// @brief Set callback for OTA completion
  void SetOTACompleteCallback(OTACompleteCallback callback);

  /// @brief Get server port
  uint16_t GetPort() const;

 private:
  /// HTTP request handlers
  static esp_err_t RootHandler(httpd_req_t* req);
  static esp_err_t StatusHandler(httpd_req_t* req);
  static esp_err_t OTAUploadHandler(httpd_req_t* req);
  static esp_err_t RebootHandler(httpd_req_t* req);

  /// Register all URI handlers
  bool RegisterHandlers();

  httpd_handle_t _server = nullptr;
  HttpServerState _state = HttpServerState::kStopped;
  HttpServerConfig _config;
  OTAProgressCallback _ota_progress_cb;
  OTACompleteCallback _ota_complete_cb;

  /// Static instance pointer for callbacks
  static HttpServer* _instance;
};

}  // namespace networking
