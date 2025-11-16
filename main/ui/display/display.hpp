#pragma once

#include <esp_err.h>
#include <lvgl.h>

#include <memory>
#include <mutex>

extern "C" {
#include <pubsub.h>
}

namespace toothless {

static constexpr uint32_t kLvglTickPeriodMs = 2;                            ///< LVGL tick period in milliseconds
static constexpr uint32_t kLvglTaskMaxDelayMs = 500;                        ///< Maximum LVGL task delay in milliseconds
static constexpr uint32_t kLvglTaskMinDelayMs = 1000 / CONFIG_FREERTOS_HZ;  ///< Minimum LVGL task delay in milliseconds
static constexpr uint8_t kLvglTaskPriority = 2;                             ///< LVGL task priority

/// @brief Display resolution information
struct DisplayResolution {
  uint16_t width;
  uint16_t height;
  bool is_portrait;
};

/// @brief Display device interface - abstraction over hardware drivers
class Display {
 public:
  /// @brief Initialize the display hardware and LVGL
  /// Selects driver based on Kconfig CONFIG_IMPL_*
  static esp_err_t Init();

  /// @brief Setup the display panel (hardware-specific)
  static esp_err_t SetupDisplayPanel();

  static esp_err_t RegisterCallbacks();

  /// @brief Setup the touchscreen panel (if enabled)
  static esp_err_t SetupTouchPanel();

  /// @brief Get the mutex for LVGL thread safety
  static std::mutex& GetLvglMutex();

  /// @brief Get the LVGL display pointer
  static lv_display_t* GetDisplayPtr();

  /// @brief Get resolution info for current display
  static DisplayResolution GetResolution();

  /// @brief Get horizontal resolution
  static uint16_t GetWidth();

  /// @brief Get vertical resolution
  static uint16_t GetHeight();

  /// @brief Check if display is in portrait orientation
  static bool IsPortrait();

 private:
  // static lv_display_t* _display;  // no shared_ptr, this is an lvgl managed object

  // Resolution info set during Init()
  static DisplayResolution _resolution;

  static void LvglTickCallback(void* arg);
  static void LvglPortTask(void* arg);
};

}  // namespace toothless