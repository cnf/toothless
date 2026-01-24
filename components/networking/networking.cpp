#include "networking.hpp"

#include "funlog.h"
#include "http_server.hpp"
#include "ota_handler.hpp"
#include "wifi_handler.hpp"

extern "C" {
#include <pubsub.h>
}

namespace networking {

// PubSub topics for network events
static constexpr const char* kTopicNetWiFiState = "net.wifi.state";
static constexpr const char* kTopicNetOTAState = "net.ota.state";
static constexpr const char* kTopicNetOTAProgress = "net.ota.progress";

NetworkManager::NetworkManager()
    : _wifi(std::make_unique<WiFiHandler>()),
      _ota(std::make_unique<OTAHandler>()),
      _http(std::make_unique<HttpServer>()) {}

NetworkManager::~NetworkManager() { Shutdown(); }

void NetworkManager::Init() {
  // esp_log_level_set(FLOG_SHORT_FILENAME, ESP_LOG_DEBUG);
  // Placeholder for any pre-startup initialization if needed
  // Set up Config
  _config = std::make_shared<SettingsMap>();
  _config_entries = new ConfigEntries;
  _config_entries->insert(std::end(*_config_entries), std::begin(config_entries), std::end(config_entries));

  // Register Config
  RegisterConfig(_config_entries, "network");
  FLOG_DEBUG("Waiting for settings...");
  GetSettings(_config, "network");

  _subscription = ps_new_subscriber(10, PS_STRLIST("network"));

  ApplySettings();
}

void NetworkManager::Loop() {
  bool new_settings = false;
  ps_msg_t* msg;
  for ((msg = ps_get(_subscription, 0)); msg != NULL; (msg = ps_get(_subscription, 0))) {
    if (ps_has_topic_suffix(msg, kTopicConfigGet) && PS_IS_NIL(msg)) {
      FLOG_DEBUG("Sending network config map");
      GetSettings(_config, "network");
    } else if (ps_has_topic_suffix(msg, kTopicConfigSet) && PS_IS_NIL(msg)) {
      FLOG_DEBUG("config set?");
      // GetSettings(_config, "network");
    } else if (ps_has_topic_suffix(msg, "ip.get") && PS_IS_NIL(msg)) {
      if (!msg->rtopic) {
        ps_unref_msg(msg);
        continue;
      }
      std::string ip = _wifi->GetIPAddress();
      FLOG_DEBUG("Publishing IP address: %s", ip.c_str());
      PS_PUB_STR(msg->rtopic, ip.c_str());
    }
  };
  ps_unref_msg(msg);
  if (new_settings) {
    ApplySettings();
  }
}

bool NetworkManager::Startup() {
  if (_initialized) {
    FLOG_WARN("NetworkManager already initialized");
    return true;
  }

  FLOG_INFO("Initializing NetworkManager");

  if (!_wifi->Init()) {
    FLOG_ERROR("Failed to initialize WiFi handler");
    return false;
  }

  // Set up WiFi state callback to publish to pubsub
  _wifi->SetStateCallback([](WiFiState state) {
    int state_val = static_cast<int>(state);
    PS_PUB_INT(kTopicNetWiFiState, state_val);
    // ps_publish(kTopicNetWiFiState, PUB_INT_T, state_val);
  });

  if (!_ota->Init()) {
    FLOG_ERROR("Failed to initialize OTA handler");
    return false;
  }

  // Set up OTA callbacks
  _ota->SetStateCallback([](OTAState state) {
    int state_val = static_cast<int>(state);
    PS_PUB_INT(kTopicNetOTAState, state_val);
    // ps_publish(kTopicNetOTAState, PUB_INT_T, state_val);
  });

  _ota->SetProgressCallback([](const OTAProgress& progress) {
    PS_PUB_INT(kTopicNetOTAProgress, progress.percent);
    // ps_publish(kTopicNetOTAProgress, PUB_INT_T, static_cast<int>(progress.percent));
  });

  _initialized = true;
  FLOG_INFO("NetworkManager initialized");
  return true;
}

void NetworkManager::Shutdown() {
  if (!_initialized) return;

  FLOG_INFO("Shutting down NetworkManager");

  StopHttpServer();
  _wifi->Disconnect();
  _wifi->Deinit();

  _initialized = false;
}

bool NetworkManager::Connect() {
  if (!_initialized) {
    FLOG_ERROR("NetworkManager not initialized");
    return false;
  }
  WiFiConfig config;
  config.ssid = std::get<std::string>(_config->at("ssid"));
  config.password = std::get<std::string>(_config->at("password"));
  config.hostname = std::get<std::string>(_config->at("hostname"));
  // config.auto_reconnect = _config->get_bool("wifi.auto_reconnect", true);
  // config.max_retries = static_cast<uint8_t>(_config->get_int("wifi.max_retries", 5));
  return _wifi->Connect(config);
}

void NetworkManager::Disconnect() {
  if (_initialized) {
    _wifi->Disconnect();
  }
}

WiFiState NetworkManager::GetWiFiState() const { return _wifi ? _wifi->GetState() : WiFiState::kDisconnected; }

bool NetworkManager::IsConnected() const { return GetWiFiState() == WiFiState::kConnected; }

bool NetworkManager::StartOTA(const std::string& url) {
  if (!_initialized) {
    FLOG_ERROR("NetworkManager not initialized");
    return false;
  }
  if (!IsConnected()) {
    FLOG_ERROR("Cannot start OTA: not connected to WiFi");
    return false;
  }
  return _ota->Start(url);
}

void NetworkManager::ConfigureOTA(const OTAConfig& config) {
  if (_ota) {
    _ota->Configure(config);
  }
}

OTAState NetworkManager::GetOTAState() const { return _ota ? _ota->GetState() : OTAState::kIdle; }

bool NetworkManager::StartHttpServer(uint16_t port) {
  if (!_initialized) {
    FLOG_ERROR("NetworkManager not initialized");
    return false;
  }
  if (!IsConnected()) {
    FLOG_WARN("Starting HTTP server without WiFi connection");
  }

  HttpServerConfig config;
  config.port = port;

  // Set up OTA callbacks on HTTP server
  _http->SetOTAProgressCallback([](size_t written, size_t total) {
    int percent = static_cast<int>((written * 100) / total);
    FLOG_DEBUG("HTTP OTA progress: %d%%", percent);
  });

  _http->SetOTACompleteCallback([](bool success) {
    if (success) {
      FLOG_INFO("HTTP OTA complete");
    } else {
      FLOG_ERROR("HTTP OTA failed");
    }
  });

  _http->EnableFileServing("/storage");

  return _http->Start(config);
}

void NetworkManager::StopHttpServer() {
  if (_http) {
    _http->Stop();
  }
}

bool NetworkManager::IsHttpServerRunning() const { return _http && _http->IsRunning(); }

esp_err_t NetworkManager::ApplySettings() {
  if (!std::get<bool>(_config->at("enabled"))) {
    Disconnect();
    Shutdown();
    return ESP_OK;
  }
  if (!_initialized) {
    if (!Startup()) {
      return ESP_FAIL;
    }
  }
  Connect();
  _wifi->SetHostname(std::get<std::string>(_config->at("hostname")));
  StartHttpServer();

  return ESP_OK;
}

}  // namespace networking
