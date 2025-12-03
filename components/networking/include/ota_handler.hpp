#pragma once

#include <esp_ota_ops.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include <functional>
#include <string>

namespace networking {

enum class OTAState;
struct OTAConfig;

/// @brief Progress info for OTA updates
struct OTAProgress {
  size_t bytes_written = 0;
  size_t total_size = 0;
  uint8_t percent = 0;
};

/// @brief Handles OTA firmware updates
/// @details Supports HTTP/HTTPS firmware downloads with progress reporting.
class OTAHandler {
 public:
  using StateCallback = std::function<void(OTAState)>;
  using ProgressCallback = std::function<void(const OTAProgress&)>;

  OTAHandler();
  ~OTAHandler();

  /// @brief Initialize OTA subsystem
  /// @return true on success
  bool Init();

  /// @brief Configure OTA settings
  void Configure(const OTAConfig& config);

  /// @brief Start OTA update
  /// @param url Firmware URL (uses configured URL if empty)
  /// @return true if OTA started
  bool Start(const std::string& url = "");

  /// @brief Abort ongoing OTA update
  void Abort();

  /// @brief Get current OTA state
  OTAState GetState() const;

  /// @brief Get current progress
  OTAProgress GetProgress() const;

  /// @brief Set callback for state changes
  void SetStateCallback(StateCallback callback);

  /// @brief Set callback for progress updates
  void SetProgressCallback(ProgressCallback callback);

  /// @brief Get running firmware version
  static std::string GetCurrentVersion();

  /// @brief Get partition info for running app
  static const esp_partition_t* GetRunningPartition();

 private:
  static void OTATask(void* arg);
  void DoOTA(const std::string& url);
  void PublishState(OTAState state);
  void PublishProgress(const OTAProgress& progress);

  OTAState _state;
  OTAConfig* _config = nullptr;
  OTAProgress _progress;
  StateCallback _state_callback;
  ProgressCallback _progress_callback;
  TaskHandle_t _task_handle = nullptr;
  std::string _pending_url;
  bool _abort_requested = false;
};

}  // namespace networking
