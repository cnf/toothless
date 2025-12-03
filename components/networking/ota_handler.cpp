#include "ota_handler.hpp"

#include <esp_app_format.h>
#include <esp_http_client.h>
#include <esp_https_ota.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include <cstring>

#include "funlog.h"
#include "networking.hpp"

extern "C" {
#include <pubsub.h>
}

namespace networking {

static constexpr size_t kOTABufferSize = 4096;
static constexpr size_t kOTATaskStackSize = 8192;

OTAHandler::OTAHandler() : _state(OTAState::kIdle) {}

OTAHandler::~OTAHandler() { Abort(); }

bool OTAHandler::Init() {
  FLOG_INFO("OTA handler initialized");
  return true;
}

void OTAHandler::Configure(const OTAConfig& config) {
  delete _config;
  _config = new OTAConfig(config);
  FLOG_INFO("OTA configured, URL: %s", config.url.c_str());
}

bool OTAHandler::Start(const std::string& url) {
  if (_state == OTAState::kDownloading || _state == OTAState::kInstalling) {
    FLOG_WARN("OTA already in progress");
    return false;
  }

  std::string target_url = url.empty() ? (_config ? _config->url : "") : url;
  if (target_url.empty()) {
    FLOG_ERROR("No OTA URL specified");
    return false;
  }

  FLOG_INFO("Starting OTA from: %s", target_url.c_str());

  _pending_url = target_url;
  _abort_requested = false;
  _progress = OTAProgress{};

  // Create OTA task
  BaseType_t result = xTaskCreate(OTATask, "ota_task", kOTATaskStackSize, this, 5, &_task_handle);
  if (result != pdPASS) {
    FLOG_ERROR("Failed to create OTA task");
    return false;
  }

  return true;
}

void OTAHandler::Abort() {
  if (_state == OTAState::kDownloading || _state == OTAState::kInstalling) {
    FLOG_WARN("Aborting OTA");
    _abort_requested = true;
  }
}

OTAState OTAHandler::GetState() const { return _state; }

OTAProgress OTAHandler::GetProgress() const { return _progress; }

void OTAHandler::SetStateCallback(StateCallback callback) { _state_callback = std::move(callback); }

void OTAHandler::SetProgressCallback(ProgressCallback callback) { _progress_callback = std::move(callback); }

std::string OTAHandler::GetCurrentVersion() {
  const esp_app_desc_t* app_desc = esp_app_get_description();
  return app_desc ? std::string(app_desc->version) : "unknown";
}

const esp_partition_t* OTAHandler::GetRunningPartition() { return esp_ota_get_running_partition(); }

void OTAHandler::OTATask(void* arg) {
  auto* handler = static_cast<OTAHandler*>(arg);
  handler->DoOTA(handler->_pending_url);
  handler->_task_handle = nullptr;
  vTaskDelete(nullptr);
}

void OTAHandler::DoOTA(const std::string& url) {
  PublishState(OTAState::kChecking);

  esp_http_client_config_t http_cfg = {};
  http_cfg.url = url.c_str();
  http_cfg.timeout_ms = 10000;
  http_cfg.keep_alive_enable = true;

  esp_https_ota_config_t ota_cfg = {};
  ota_cfg.http_config = &http_cfg;

  esp_https_ota_handle_t ota_handle = nullptr;
  esp_err_t err = esp_https_ota_begin(&ota_cfg, &ota_handle);
  if (err != ESP_OK) {
    FLOG_ERROR("OTA begin failed: %s", esp_err_to_name(err));
    PublishState(OTAState::kError);
    return;
  }

  // Get image size for progress
  _progress.total_size = esp_https_ota_get_image_size(ota_handle);
  FLOG_INFO("OTA image size: %zu bytes", _progress.total_size);

  PublishState(OTAState::kDownloading);

  while (!_abort_requested) {
    err = esp_https_ota_perform(ota_handle);
    if (err == ESP_ERR_HTTPS_OTA_IN_PROGRESS) {
      _progress.bytes_written = esp_https_ota_get_image_len_read(ota_handle);
      if (_progress.total_size > 0) {
        _progress.percent = static_cast<uint8_t>((_progress.bytes_written * 100) / _progress.total_size);
      }
      PublishProgress(_progress);
      continue;
    }
    break;
  }

  if (_abort_requested) {
    FLOG_WARN("OTA aborted by user");
    esp_https_ota_abort(ota_handle);
    PublishState(OTAState::kIdle);
    return;
  }

  if (err != ESP_OK) {
    FLOG_ERROR("OTA failed: %s", esp_err_to_name(err));
    esp_https_ota_abort(ota_handle);
    PublishState(OTAState::kError);
    return;
  }

  PublishState(OTAState::kInstalling);

  if (!esp_https_ota_is_complete_data_received(ota_handle)) {
    FLOG_ERROR("OTA data incomplete");
    esp_https_ota_abort(ota_handle);
    PublishState(OTAState::kError);
    return;
  }

  err = esp_https_ota_finish(ota_handle);
  if (err != ESP_OK) {
    if (err == ESP_ERR_OTA_VALIDATE_FAILED) {
      FLOG_ERROR("OTA image validation failed");
    } else {
      FLOG_ERROR("OTA finish failed: %s", esp_err_to_name(err));
    }
    PublishState(OTAState::kError);
    return;
  }

  FLOG_INFO("OTA complete, restarting...");
  PublishState(OTAState::kComplete);

  vTaskDelay(pdMS_TO_TICKS(1000));
  esp_restart();
}

void OTAHandler::PublishState(OTAState state) {
  _state = state;
  if (_state_callback) {
    _state_callback(state);
  }
}

void OTAHandler::PublishProgress(const OTAProgress& progress) {
  if (_progress_callback) {
    _progress_callback(progress);
  }
}

}  // namespace networking
