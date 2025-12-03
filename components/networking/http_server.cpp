#include "http_server.hpp"

#include <esp_app_format.h>
#include <esp_ota_ops.h>
#include <esp_system.h>

#include <cstring>
#include <memory>

#include "funlog.h"

extern "C" {
#include <pubsub.h>
}

namespace networking {

// Static instance for callbacks
HttpServer* HttpServer::_instance = nullptr;

// Simple HTML page for OTA upload
static const char* kOTAUploadPage = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>Toothless OTA Update</title>
  <style>
    body { font-family: sans-serif; max-width: 600px; margin: 50px auto; padding: 20px; background: #1a1a2e; color: #eee; }
    h1 { color: #ff6b6b; }
    .upload-form { background: #16213e; padding: 20px; border-radius: 8px; margin: 20px 0; }
    input[type="file"] { margin: 10px 0; }
    button { background: #ff6b6b; color: white; border: none; padding: 12px 24px; border-radius: 4px; cursor: pointer; font-size: 16px; }
    button:hover { background: #ee5a5a; }
    button:disabled { background: #666; cursor: not-allowed; }
    .progress { width: 100%; height: 24px; background: #333; border-radius: 4px; margin: 10px 0; display: none; }
    .progress-bar { height: 100%; background: #4ecdc4; border-radius: 4px; width: 0%; transition: width 0.3s; }
    .status { margin: 10px 0; padding: 10px; border-radius: 4px; }
    .info { background: #16213e; border-left: 4px solid #4ecdc4; }
    .error { background: #2d1b1b; border-left: 4px solid #ff6b6b; }
    .success { background: #1b2d1b; border-left: 4px solid #6bff6b; }
  </style>
</head>
<body>
  <h1>🔥 Toothless OTA</h1>
  <div class="status info" id="version">Loading...</div>
  <div class="upload-form">
    <h3>Upload Firmware</h3>
    <form id="upload-form" enctype="multipart/form-data">
      <input type="file" id="firmware" name="firmware" accept=".bin">
      <br><br>
      <button type="submit" id="upload-btn">Upload & Flash</button>
    </form>
    <div class="progress" id="progress">
      <div class="progress-bar" id="progress-bar"></div>
    </div>
    <div class="status" id="status" style="display:none"></div>
  </div>
  <script>
    fetch('/api/status').then(r => r.json()).then(d => {
      document.getElementById('version').innerHTML = 
        '<b>Version:</b> ' + d.version + ' | <b>Free heap:</b> ' + d.free_heap + ' bytes';
    });
    document.getElementById('upload-form').addEventListener('submit', async (e) => {
      e.preventDefault();
      const file = document.getElementById('firmware').files[0];
      if (!file) { alert('Select a firmware file'); return; }
      const btn = document.getElementById('upload-btn');
      const progress = document.getElementById('progress');
      const progressBar = document.getElementById('progress-bar');
      const status = document.getElementById('status');
      btn.disabled = true;
      progress.style.display = 'block';
      status.style.display = 'none';
      const xhr = new XMLHttpRequest();
      xhr.open('POST', '/api/ota', true);
      xhr.upload.onprogress = (e) => {
        if (e.lengthComputable) {
          const pct = Math.round((e.loaded / e.total) * 100);
          progressBar.style.width = pct + '%';
        }
      };
      xhr.onload = () => {
        btn.disabled = false;
        status.style.display = 'block';
        if (xhr.status === 200) {
          status.className = 'status success';
          status.innerHTML = 'Upload complete! Rebooting in 3 seconds...';
          setTimeout(() => { window.location.reload(); }, 5000);
        } else {
          status.className = 'status error';
          status.innerHTML = 'Upload failed: ' + xhr.responseText;
        }
      };
      xhr.onerror = () => {
        btn.disabled = false;
        status.style.display = 'block';
        status.className = 'status error';
        status.innerHTML = 'Network error';
      };
      xhr.send(file);
    });
  </script>
</body>
</html>
)rawliteral";

HttpServer::HttpServer() { _instance = this; }

HttpServer::~HttpServer() {
  Stop();
  if (_instance == this) {
    _instance = nullptr;
  }
}

bool HttpServer::Start(const HttpServerConfig& config) {
  if (_server != nullptr) {
    FLOG_WARN("HTTP server already running");
    return true;
  }

  _config = config;
  FLOG_INFO("Starting HTTP server on port %d", _config.port);

  httpd_config_t httpd_config = HTTPD_DEFAULT_CONFIG();
  httpd_config.server_port = _config.port;
  httpd_config.max_uri_handlers = _config.max_uri_handlers;
  httpd_config.stack_size = _config.stack_size;
  httpd_config.lru_purge_enable = true;

  esp_err_t err = httpd_start(&_server, &httpd_config);
  if (err != ESP_OK) {
    FLOG_ERROR("Failed to start HTTP server: %s", esp_err_to_name(err));
    _state = HttpServerState::kError;
    return false;
  }

  if (!RegisterHandlers()) {
    FLOG_ERROR("Failed to register HTTP handlers");
    httpd_stop(_server);
    _server = nullptr;
    _state = HttpServerState::kError;
    return false;
  }

  _state = HttpServerState::kRunning;
  FLOG_INFO("HTTP server started on port %d", _config.port);
  return true;
}

void HttpServer::Stop() {
  if (_server == nullptr) return;

  FLOG_INFO("Stopping HTTP server");
  httpd_stop(_server);
  _server = nullptr;
  _state = HttpServerState::kStopped;
}

bool HttpServer::IsRunning() const { return _server != nullptr; }

HttpServerState HttpServer::GetState() const { return _state; }

void HttpServer::SetOTAProgressCallback(OTAProgressCallback callback) { _ota_progress_cb = std::move(callback); }

void HttpServer::SetOTACompleteCallback(OTACompleteCallback callback) { _ota_complete_cb = std::move(callback); }

uint16_t HttpServer::GetPort() const { return _config.port; }

bool HttpServer::RegisterHandlers() {
  // Root handler - serves OTA upload page
  httpd_uri_t root = {.uri = "/", .method = HTTP_GET, .handler = RootHandler, .user_ctx = nullptr};

  // Status API
  httpd_uri_t status = {.uri = "/api/status", .method = HTTP_GET, .handler = StatusHandler, .user_ctx = nullptr};

  // OTA upload endpoint
  httpd_uri_t ota = {.uri = "/api/ota", .method = HTTP_POST, .handler = OTAUploadHandler, .user_ctx = nullptr};

  // Reboot endpoint
  httpd_uri_t reboot = {.uri = "/api/reboot", .method = HTTP_POST, .handler = RebootHandler, .user_ctx = nullptr};

  esp_err_t err;
  err = httpd_register_uri_handler(_server, &root);
  if (err != ESP_OK) return false;

  err = httpd_register_uri_handler(_server, &status);
  if (err != ESP_OK) return false;

  err = httpd_register_uri_handler(_server, &ota);
  if (err != ESP_OK) return false;

  err = httpd_register_uri_handler(_server, &reboot);
  if (err != ESP_OK) return false;

  return true;
}

esp_err_t HttpServer::RootHandler(httpd_req_t* req) {
  httpd_resp_set_type(req, "text/html");
  httpd_resp_send(req, kOTAUploadPage, strlen(kOTAUploadPage));
  return ESP_OK;
}

esp_err_t HttpServer::StatusHandler(httpd_req_t* req) {
  const esp_app_desc_t* app_desc = esp_app_get_description();
  const esp_partition_t* running = esp_ota_get_running_partition();

  char response[256];
  snprintf(response, sizeof(response), "{\"version\":\"%s\",\"idf_ver\":\"%s\",\"partition\":\"%s\",\"free_heap\":%lu}",
           app_desc ? app_desc->version : "unknown", app_desc ? app_desc->idf_ver : "unknown",
           running ? running->label : "unknown", esp_get_free_heap_size());

  httpd_resp_set_type(req, "application/json");
  httpd_resp_send(req, response, strlen(response));
  return ESP_OK;
}

esp_err_t HttpServer::OTAUploadHandler(httpd_req_t* req) {
  FLOG_INFO("OTA upload started, size: %d bytes", req->content_len);

  // Get update partition
  const esp_partition_t* update_partition = esp_ota_get_next_update_partition(nullptr);
  if (update_partition == nullptr) {
    FLOG_ERROR("No OTA partition found");
    httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "No OTA partition");
    return ESP_FAIL;
  }

  FLOG_INFO("Writing to partition: %s", update_partition->label);

  esp_ota_handle_t ota_handle;
  esp_err_t err = esp_ota_begin(update_partition, OTA_SIZE_UNKNOWN, &ota_handle);
  if (err != ESP_OK) {
    FLOG_ERROR("esp_ota_begin failed: %s", esp_err_to_name(err));
    httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "OTA begin failed");
    return ESP_FAIL;
  }

  // Buffer for receiving data
  static constexpr size_t kBufferSize = 4096;
  auto buffer = std::make_unique<char[]>(kBufferSize);

  size_t total_written = 0;
  size_t total_size = req->content_len;
  int received;

  while (total_written < total_size) {
    received = httpd_req_recv(req, buffer.get(), std::min(kBufferSize, total_size - total_written));
    if (received <= 0) {
      if (received == HTTPD_SOCK_ERR_TIMEOUT) {
        continue;  // Retry on timeout
      }
      FLOG_ERROR("Error receiving data");
      esp_ota_abort(ota_handle);
      httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Receive error");
      return ESP_FAIL;
    }

    err = esp_ota_write(ota_handle, buffer.get(), received);
    if (err != ESP_OK) {
      FLOG_ERROR("esp_ota_write failed: %s", esp_err_to_name(err));
      esp_ota_abort(ota_handle);
      httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Write failed");
      return ESP_FAIL;
    }

    total_written += received;

    // Progress callback
    if (_instance && _instance->_ota_progress_cb) {
      _instance->_ota_progress_cb(total_written, total_size);
    }

    // Publish progress via pubsub
    int percent = static_cast<int>((total_written * 100) / total_size);
    PS_PUB_INT("net.ota.progress", percent);
  }

  FLOG_INFO("OTA receive complete, validating...");

  err = esp_ota_end(ota_handle);
  if (err != ESP_OK) {
    if (err == ESP_ERR_OTA_VALIDATE_FAILED) {
      FLOG_ERROR("OTA image validation failed");
    } else {
      FLOG_ERROR("esp_ota_end failed: %s", esp_err_to_name(err));
    }
    httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Validation failed");
    if (_instance && _instance->_ota_complete_cb) {
      _instance->_ota_complete_cb(false);
    }
    return ESP_FAIL;
  }

  err = esp_ota_set_boot_partition(update_partition);
  if (err != ESP_OK) {
    FLOG_ERROR("esp_ota_set_boot_partition failed: %s", esp_err_to_name(err));
    httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Set boot partition failed");
    if (_instance && _instance->_ota_complete_cb) {
      _instance->_ota_complete_cb(false);
    }
    return ESP_FAIL;
  }

  FLOG_INFO("OTA successful! Rebooting...");

  // Notify completion
  if (_instance && _instance->_ota_complete_cb) {
    _instance->_ota_complete_cb(true);
  }
  PS_PUB_INT("net.ota.state", 4);  // kComplete

  httpd_resp_sendstr(req, "OTA successful, rebooting...");

  // Schedule reboot
  vTaskDelay(pdMS_TO_TICKS(1000));
  esp_restart();

  return ESP_OK;
}

esp_err_t HttpServer::RebootHandler(httpd_req_t* req) {
  FLOG_INFO("Reboot requested via HTTP");
  httpd_resp_sendstr(req, "Rebooting...");
  vTaskDelay(pdMS_TO_TICKS(500));
  esp_restart();
  return ESP_OK;
}

}  // namespace networking
