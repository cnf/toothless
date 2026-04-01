#include "http_server.hpp"

#include <dirent.h>
#include <esp_app_format.h>
#include <esp_ota_ops.h>
#include <esp_system.h>
#include <sys/stat.h>

#include <cstring>
#include <memory>

#include "funlog.h"

extern "C" {
#include <pubsub.h>
}

namespace networking {

// Static instance for callbacks
HttpServer* HttpServer::_instance = nullptr;

// extern const uint8_t favicon_ico_start[] asm("_binary_favicon_ico_start");
// extern const uint8_t favicon_ico_end[] asm("_binary_favicon_ico_end");

// Welcome page with navigation
static const char* kWelcomePage = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <link rel="shortcut icon" href="/favicon.ico">
  <title>Toothless Control</title>
  <style>
    body { font-family: sans-serif; max-width: 600px; margin: 50px auto; padding: 20px; background: #1a1a2e; color: #eee; }
    h1 { color: #ff6b6b; text-align: center; }
    .nav-grid { display: grid; grid-template-columns: repeat(auto-fit, minmax(250px, 1fr)); gap: 15px; margin: 30px 0; }
    .nav-button { background: #16213e; padding: 25px; border-radius: 8px; text-align: center; text-decoration: none; color: #eee; transition: all 0.3s; border: 2px solid transparent; }
    .nav-button:hover { background: #0f3460; border-color: #4ecdc4; transform: translateY(-2px); }
    .nav-icon { font-size: 48px; margin-bottom: 10px; }
    .nav-title { font-size: 20px; font-weight: bold; margin-bottom: 5px; }
    .nav-desc { font-size: 14px; color: #aaa; }
    .status { margin: 20px 0; padding: 15px; border-radius: 4px; background: #16213e; border-left: 4px solid #4ecdc4; }
    .status-item { margin: 5px 0; }
  </style>
</head>
<body>
  <h1>🔥 Toothless Control</h1>
  <div class="status" id="status">
    <div class="status-item"><b>Version:</b> <span id="version">Loading...</span></div>
    <div class="status-item"><b>Free Heap:</b> <span id="heap">Loading...</span></div>
    <div class="status-item"><b>Partition:</b> <span id="partition">Loading...</span></div>
  </div>
  <div class="nav-grid">
    <a href="/ota" class="nav-button">
      <div class="nav-icon">📦</div>
      <div class="nav-title">OTA Update</div>
      <div class="nav-desc">Upload firmware</div>
    </a>
    <a href="/files" class="nav-button">
      <div class="nav-icon">📁</div>
      <div class="nav-title">Files</div>
      <div class="nav-desc">Browse files</div>
    </a>
    <a href="/api/status" class="nav-button">
      <div class="nav-icon">ℹ️</div>
      <div class="nav-title">Status API</div>
      <div class="nav-desc">JSON status info</div>
    </a>
    <a href="/logs" class="nav-button">
      <div class="nav-icon">🪵</div>
      <div class="nav-title">Logs</div>
      <div class="nav-desc">Live log output</div>
    </a>
    <a href="#" onclick="reboot(); return false;" class="nav-button">
      <div class="nav-icon">🔄</div>
      <div class="nav-title">Reboot</div>
      <div class="nav-desc">Restart device</div>
    </a>
  </div>
  <script>
    fetch('/api/status').then(r => r.json()).then(d => {
      document.getElementById('version').textContent = d.version;
      document.getElementById('heap').textContent = d.free_heap + ' bytes';
      document.getElementById('partition').textContent = d.partition;
    }).catch(e => {
      document.getElementById('version').textContent = 'Error';
      document.getElementById('heap').textContent = 'Error';
      document.getElementById('partition').textContent = 'Error';
    });
    function reboot() {
      if (confirm('Reboot device?')) {
        fetch('/api/reboot', {method: 'POST'}).then(() => {
          alert('Rebooting...');
          setTimeout(() => window.location.reload(), 5000);
        });
      }
    }
  </script>
</body>
</html>
)rawliteral";

// OTA upload page
static const char* kOTAUploadPage = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <link rel="shortcut icon" href="/favicon.ico">
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

static const char* kLogPage = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <link rel="shortcut icon" href="/favicon.ico">
  <title>Toothless Log</title>
  <style>
    body { font-family: monospace; background: #0d0d0d; color: #ccc; margin: 0; padding: 10px; }
    h1 { color: #ff6b6b; font-family: sans-serif; margin: 0 0 8px 0; }
    #controls { font-family: sans-serif; margin-bottom: 8px; display: flex; gap: 10px; align-items: center; }
    button { background: #16213e; color: #eee; border: 1px solid #4ecdc4; padding: 6px 14px;
             border-radius: 4px; cursor: pointer; }
    button:hover { background: #0f3460; }
    #log { background: #111; border: 1px solid #333; padding: 8px; height: calc(100vh - 90px);
           overflow-y: auto; white-space: pre-wrap; word-break: break-all; font-size: 13px; }
    .E { color: #ff6b6b; }
    .W { color: #ffd93d; }
    .I { color: #c3c3c3; }
    .D { color: #6bcbff; }
  </style>
</head>
<body>
  <h1>🪵 Log</h1>
  <div id="controls">
    <button onclick="togglePause()">⏸ Pause</button>
    <button onclick="clearLog()">🗑 Clear</button>
    <button onclick="window.location='/'">🏠 Home</button>
    <span id="status" style="color:#aaa; font-size:13px"></span>
  </div>
  <div id="log"></div>
  <script>
    let since = 0, paused = false, count = 0;
    const el = document.getElementById('log');
    const levelClass = { 1:'E', 2:'W', 3:'I', 4:'D', 5:'D' };
    const levelLabel = { 1:'E', 2:'W', 3:'I', 4:'D', 5:'V' };
    function togglePause() {
      paused = !paused;
      document.querySelector('button').textContent = paused ? '▶ Resume' : '⏸ Pause';
    }
    function clearLog() { el.innerHTML = ''; count = 0; }
    function appendLines(entries) {
      const atBottom = el.scrollHeight - el.scrollTop <= el.clientHeight + 40;
      entries.forEach(e => {
        const cls = levelClass[e.level] || 'I';
        const lbl = levelLabel[e.level] || '?';
        const line = document.createElement('span');
        line.className = cls;
        line.textContent = '[' + lbl + '][' + e.tag + '] ' + e.msg + '\n';
        el.appendChild(line);
        count++;
      });
      if (count > 2000) {
        while (el.children.length > 1500) el.removeChild(el.firstChild);
        count = 1500;
      }
      if (atBottom) el.scrollTop = el.scrollHeight;
    }
    async function poll() {
      if (!paused) {
        try {
          const r = await fetch('/api/logs?since=' + since);
          if (r.ok) {
            const d = await r.json();
            if (d.entries && d.entries.length) appendLines(d.entries);
            since = d.next_seq;
            document.getElementById('status').textContent =
              'seq:' + since + ' | ' + new Date().toLocaleTimeString();
          }
        } catch(e) {
          document.getElementById('status').textContent = 'error: ' + e.message;
        }
      }
      setTimeout(poll, 2000);
    }
    poll();
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
  httpd_config.uri_match_fn = httpd_uri_match_wildcard;

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
  // Root handler - serves welcome page
  httpd_uri_t root = {.uri = "/", .method = HTTP_GET, .handler = RootHandler, .user_ctx = nullptr};

  // Favicon handler - serves embedded favicon
  httpd_uri_t favicon = {.uri = "/favicon.ico", .method = HTTP_GET, .handler = FaviconHandler, .user_ctx = nullptr};

  // OTA page handler
  httpd_uri_t ota_page = {.uri = "/ota", .method = HTTP_GET, .handler = OTAPageHandler, .user_ctx = nullptr};

  // Status API
  httpd_uri_t status = {.uri = "/api/status", .method = HTTP_GET, .handler = StatusHandler, .user_ctx = nullptr};

  // OTA upload endpoint
  httpd_uri_t ota = {.uri = "/api/ota", .method = HTTP_POST, .handler = OTAUploadHandler, .user_ctx = nullptr};

  // Reboot endpoint
  httpd_uri_t reboot = {.uri = "/api/reboot", .method = HTTP_POST, .handler = RebootHandler, .user_ctx = nullptr};

  // File serving (if enabled)
  if (!_file_serve_path.empty()) {
    httpd_uri_t files = {.uri = "/files", .method = HTTP_GET, .handler = FileListHandler, .user_ctx = nullptr};
    httpd_uri_t download = {.uri = "/files/*", .method = HTTP_GET, .handler = FileDownloadHandler, .user_ctx = nullptr};
    httpd_register_uri_handler(_server, &files);
    httpd_register_uri_handler(_server, &download);
  }

  esp_err_t err;
  err = httpd_register_uri_handler(_server, &root);
  if (err != ESP_OK) return false;

  err = httpd_register_uri_handler(_server, &favicon);
  if (err != ESP_OK) return false;

  err = httpd_register_uri_handler(_server, &ota_page);
  if (err != ESP_OK) return false;

  err = httpd_register_uri_handler(_server, &status);
  if (err != ESP_OK) return false;

  err = httpd_register_uri_handler(_server, &ota);
  if (err != ESP_OK) return false;

  err = httpd_register_uri_handler(_server, &reboot);
  if (err != ESP_OK) return false;

  httpd_uri_t log_page = {.uri = "/logs", .method = HTTP_GET, .handler = LogPageHandler, .user_ctx = nullptr};
  httpd_uri_t log_api = {.uri = "/api/logs", .method = HTTP_GET, .handler = LogApiHandler, .user_ctx = nullptr};
  httpd_register_uri_handler(_server, &log_page);
  httpd_register_uri_handler(_server, &log_api);

  return true;
}

esp_err_t HttpServer::RootHandler(httpd_req_t* req) {
  httpd_resp_set_type(req, "text/html");
  httpd_resp_send(req, kWelcomePage, strlen(kWelcomePage));
  return ESP_OK;
}

/* Handler to respond with an icon file embedded in flash.
 * Browsers expect to GET website icon at URI /favicon.ico.
 * This can be overridden by uploading file with same name */
esp_err_t HttpServer::FaviconHandler(httpd_req_t* req) {
  extern const unsigned char favicon_ico_start[] asm("_binary_favicon_ico_start");
  extern const unsigned char favicon_ico_end[] asm("_binary_favicon_ico_end");
  const size_t favicon_ico_size = (favicon_ico_end - favicon_ico_start);
  httpd_resp_set_type(req, "image/x-icon");
  httpd_resp_send(req, (const char*)favicon_ico_start, favicon_ico_size);
  return ESP_OK;
}

esp_err_t HttpServer::OTAPageHandler(httpd_req_t* req) {
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

void HttpServer::EnableFileServing(const std::string& base_path) { _file_serve_path = base_path; }

esp_err_t HttpServer::FileListHandler(httpd_req_t* req) {
  if (!_instance || _instance->_file_serve_path.empty()) {
    httpd_resp_send_404(req);
    return ESP_OK;
  }

  // Get subpath from URI (after "/files")
  const char* subpath = req->uri + 6;  // Skip "/files"
  if (subpath[0] == '/') subpath++;    // Skip leading slash

  char full_path[128];
  if (strlen(subpath) > 0) {
    snprintf(full_path, sizeof(full_path), "%s/%s", _instance->_file_serve_path.c_str(), subpath);
  } else {
    snprintf(full_path, sizeof(full_path), "%s", _instance->_file_serve_path.c_str());
  }

  httpd_resp_set_type(req, "text/html");
  httpd_resp_sendstr_chunk(
      req,
      "<html><head><meta charset=\"UTF-8\"><style>"
      "body{font-family:sans-serif;background:#1a1a2e;color:#eee;max-width:600px;margin:50px auto;padding:20px;}"
      "a{color:#4ecdc4;}</style></head>"
      "<body><h1>📁 Files</h1>");

  // Show parent link if in subdirectory
  if (strlen(subpath) > 0) {
    httpd_resp_sendstr_chunk(req, "<p><a href=\"/files\">⬆️ Parent</a></p>");
  }

  httpd_resp_sendstr_chunk(req, "<ul>");

  DIR* dir = opendir(full_path);
  if (dir) {
    struct dirent* ent;
    while ((ent = readdir(dir))) {
      if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0) continue;

      char link[256];
      const char* prefix = strlen(subpath) > 0 ? subpath : "";
      const char* sep = strlen(subpath) > 0 ? "/" : "";

      if (ent->d_type == DT_DIR) {
        snprintf(link, sizeof(link), "<li>📁 <a href=\"/files/%s%s%.64s/\">%.64s/</a></li>", prefix, sep, ent->d_name,
                 ent->d_name);
      } else {
        snprintf(link, sizeof(link), "<li>📄 <a href=\"/files/%s%s%.64s\">%.64s</a></li>", prefix, sep, ent->d_name,
                 ent->d_name);
      }
      httpd_resp_sendstr_chunk(req, link);
    }
    closedir(dir);
  } else {
    httpd_resp_sendstr_chunk(req, "<li>Unable to open directory</li>");
  }

  httpd_resp_sendstr_chunk(req, "</ul></body></html>");
  httpd_resp_send_chunk(req, nullptr, 0);
  return ESP_OK;
}

esp_err_t HttpServer::FileDownloadHandler(httpd_req_t* req) {
  if (!_instance || _instance->_file_serve_path.empty()) {
    httpd_resp_send_404(req);
    return ESP_OK;
  }

  // Extract path from URI (skip "/files/")
  const char* filepath = req->uri + 7;
  char path[128];
  snprintf(path, sizeof(path), "%s/%s", _instance->_file_serve_path.c_str(), filepath);

  // Remove trailing slash if present
  size_t len = strlen(path);
  if (len > 1 && path[len - 1] == '/') path[len - 1] = '\0';

  struct stat st;
  if (stat(path, &st) != 0) {
    httpd_resp_send_404(req);
    return ESP_OK;
  }

  // If directory, redirect to list handler
  if (S_ISDIR(st.st_mode)) {
    return FileListHandler(req);
  }

  FILE* f = fopen(path, "rb");
  if (!f) {
    httpd_resp_send_404(req);
    return ESP_OK;
  }

  // Get just filename for header
  const char* filename = strrchr(filepath, '/');
  filename = filename ? filename + 1 : filepath;

  char header[128];
  snprintf(header, sizeof(header), "attachment; filename=\"%.64s\"", filename);
  httpd_resp_set_hdr(req, "Content-Disposition", header);
  httpd_resp_set_type(req, "application/octet-stream");

  char buf[512];
  size_t read;
  while ((read = fread(buf, 1, sizeof(buf), f)) > 0) {
    httpd_resp_send_chunk(req, buf, read);
  }
  httpd_resp_send_chunk(req, nullptr, 0);
  fclose(f);
  return ESP_OK;
}

void HttpServer::EnableLogStreaming(size_t ring_size) {
  _log_ring_max = ring_size;
  _log_ring.resize(ring_size);
  _log_mutex = xSemaphoreCreateMutex();
  funlog_set_net_fun_log(&HttpServer::NetLogHook);
  FLOG_INFO("Log streaming enabled, ring size: %d", static_cast<int>(ring_size));
}

// static
int HttpServer::NetLogHook(int level, const char* tag, const char* fmt, va_list args) {
  if (!_instance || !_instance->_log_mutex) return 0;
  char buf[192];
  vsnprintf(buf, sizeof(buf), fmt, args);
  _instance->PushLogEntry(level, tag, buf);
  return 0;
}

void HttpServer::PushLogEntry(int level, const char* tag, const char* msg) {
  if (xSemaphoreTake(_log_mutex, 0) != pdTRUE) return;  // drop on contention
  LogEntry& e = _log_ring[_log_ring_head % _log_ring_max];
  e.seq = ++_log_seq;
  e.level = level;
  snprintf(e.tag, sizeof(e.tag), "%s", tag ? tag : "");
  snprintf(e.msg, sizeof(e.msg), "%s", msg ? msg : "");
  _log_ring_head++;
  xSemaphoreGive(_log_mutex);
}

esp_err_t HttpServer::LogPageHandler(httpd_req_t* req) {
  httpd_resp_set_type(req, "text/html");
  httpd_resp_send(req, kLogPage, strlen(kLogPage));
  return ESP_OK;
}

esp_err_t HttpServer::LogApiHandler(httpd_req_t* req) {
  if (!_instance || !_instance->_log_mutex) {
    httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Log streaming not enabled");
    return ESP_FAIL;
  }

  char since_buf[16] = "0";
  if (httpd_req_get_url_query_len(req) > 0) {
    char query[32];
    if (httpd_req_get_url_query_str(req, query, sizeof(query)) == ESP_OK) {
      httpd_query_key_value(query, "since", since_buf, sizeof(since_buf));
    }
  }
  const uint32_t since_seq = static_cast<uint32_t>(strtoul(since_buf, nullptr, 10));

  char* out = static_cast<char*>(malloc(4096));
  if (!out) {
    httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "OOM");
    return ESP_FAIL;
  }

  xSemaphoreTake(_instance->_log_mutex, portMAX_DELAY);

  const size_t max = _instance->_log_ring_max;
  const size_t head = _instance->_log_ring_head;
  const auto cur_seq = static_cast<unsigned int>(_instance->_log_seq);

  size_t start = (head >= max) ? head - max : 0;
  int out_pos = snprintf(out, 4096, "{\"next_seq\":%u,\"entries\":[", cur_seq);

  bool first = true;
  char safe_msg[384];  // 2x LogEntry::msg size

  for (size_t i = start; i < head && out_pos < 3900; i++) {
    const LogEntry& e = _instance->_log_ring[i % max];
    if (e.seq <= since_seq) continue;
    if (!first) out[out_pos++] = ',';
    first = false;

    // Escape " and \ for JSON
    int sm = 0;
    for (const char* p = e.msg; *p && sm < static_cast<int>(sizeof(safe_msg)) - 2; p++) {
      if (*p == '"' || *p == '\\') safe_msg[sm++] = '\\';
      safe_msg[sm++] = *p;
    }
    safe_msg[sm] = '\0';

    out_pos += snprintf(out + out_pos, 4096 - out_pos, "{\"seq\":%u,\"level\":%d,\"tag\":\"%s\",\"msg\":\"%s\"}",
                        static_cast<unsigned int>(e.seq), e.level, e.tag, safe_msg);
  }

  xSemaphoreGive(_instance->_log_mutex);

  snprintf(out + out_pos, 4096 - out_pos, "]}");

  httpd_resp_set_type(req, "application/json");
  httpd_resp_send(req, out, strlen(out));
  free(out);
  return ESP_OK;
}

}  // namespace networking
