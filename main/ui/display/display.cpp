#include "display.hpp"

#include <esp_check.h>
#include <esp_log.h>
#include <esp_task_wdt.h>
#include <esp_timer.h>

#include <algorithm>

#include "config.h"
#include "display_impl.hpp"
#include "funlog.h"
#include "ui/themes/style_registry.hpp"
#include "ui/themes/widget_factories.hpp"

namespace toothless {

// Static member initialization
// DisplayResolution Display::_resolution = {0, 0, false};
// static std::unique_ptr<DisplayResolution> _resolution = nullptr;
static lv_display_t* _display_ptr = nullptr;
// static std::mutex _lvgl_mutex;

esp_err_t Display::Init() {
  FLOG_INFO("Initializing display...");

  // Initialize _resolution dynamically
  // if (!_resolution) {
  //   _resolution = std::make_unique<DisplayResolution>();
  //   _resolution->width = 0;
  //   _resolution->height = 0;
  //   _resolution->is_portrait = false;
  // }

  ESP_RETURN_ON_ERROR(display::impl::DisplayPanelSetup(), FLOG_SHORT_FILENAME, "Display panel setup failed");
  ESP_RETURN_ON_ERROR(display::impl::TouchPanelSetup(), FLOG_SHORT_FILENAME, "Touchpanel setup failed");
  ESP_RETURN_ON_ERROR(RegisterCallbacks(), FLOG_SHORT_FILENAME, "Display callback registration failed");
  _display_ptr = display::impl::GetDisplayObjPtr();
  // ui::theme::SetTheme(_display_ptr);
  themes::Init(themes::ThemeId::TOOTHLESS);
  // themes::Init(themes::ThemeId::JADE);

  // display::impl::GetDisplayDimensions(_resolution->width, _resolution->height);

  // FLOG_INFO("Display initialized: %ux%u (%s)", _resolution->width, _resolution->height,
  //           _resolution->is_portrait ? "portrait" : "landscape");

  // TODO: SetTheme();

  FLOG_DEBUG("Free heap: %u, Min free: %u", esp_get_free_heap_size(), esp_get_minimum_free_heap_size());
  ESP_RETURN_ON_FALSE(xTaskCreatePinnedToCore(LvglPortTask,               //<! Task function
                                              "LVGL",                     //<! Task name
                                              CONFIG_TL_LVGL_STACK_SIZE,  //<! Stack size from Kconfig
                                              NULL,                       //<! Task parameter
                                              kLvglTaskPriority,          //<! Task priority
                                              NULL,                       //<! Task handle
                                              0                           //<! Core ID
                                              ),
                      ESP_ERR_INVALID_STATE, FLOG_SHORT_FILENAME, "Failed to create LVGL task");
  display::impl::TurnOn();
  return ESP_OK;
}

esp_err_t Display::SetupDisplayPanel() {
  // TODO: Implement hardware-specific panel setup
  // This will be called from display driver components (display_st7796, display_new, etc.)
  // For now, placeholder that should be replaced by actual driver code
  _display_ptr = lv_display_get_default();
  if (!_display_ptr) {
    FLOG_ERROR("No LVGL display found");
    return ESP_ERR_NOT_FOUND;
  }
  return ESP_OK;
}

esp_err_t Display::RegisterCallbacks() {
  // lv_display_set_flush_cb(_display_ptr, LvglFlushCallback);

  FLOG_INFO("Install LVGL tick timer");
  {
    // Tick interface for LVGL (using esp_timer to generate 2ms periodic event)
    const esp_timer_create_args_t lvgl_tick_timer_args = {.callback = &LvglTickCallback, .name = "lvgl_tick"};
    esp_timer_handle_t lvgl_tick_timer = NULL;
    ESP_ERROR_CHECK(esp_timer_create(&lvgl_tick_timer_args, &lvgl_tick_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(lvgl_tick_timer, kLvglTickPeriodMs * 1000));
  }

  // /* Register done callback */
  // ESP_ERROR_CHECK(esp_lcd_panel_io_register_event_callbacks(io_handle, &cbs, _display_ptr));
  // // lv_display_add_event_cb(_display.get(), lvgl_display_event_cb, LV_EVENT_REFR_READY, NULL);
  return ESP_OK;
}

esp_err_t Display::SetupTouchPanel() {
  // TODO: Implement hardware-specific touchpanel setup
  // Optional - only if CONFIG_TL_HAS_TOUCHPANEL is enabled
  FLOG_DEBUG("Touchpanel setup (driver-specific)");
  return ESP_OK;
}

// std::mutex& Display::GetLvglMutex() { return _lvgl_mutex; }

lv_display_t* Display::GetDisplayPtr() { return _display_ptr; }

// DisplayResolution Display::GetResolution() {
//   if (!_resolution) {
//     FLOG_ERROR("Resolution not initialized! Call Display::Init() first.");
//     throw std::runtime_error("Resolution not initialized");
//   }
//   return *_resolution;
// }

// uint16_t Display::GetWidth() {
//   if (!_resolution) {
//     FLOG_ERROR("Resolution not initialized! Call Display::Init() first.");
//     throw std::runtime_error("Resolution not initialized");
//   }
//   return _resolution->width;
// }

// uint16_t Display::GetHeight() {
//   if (!_resolution) {
//     FLOG_ERROR("Resolution not initialized! Call Display::Init() first.");
//     throw std::runtime_error("Resolution not initialized");
//   }
//   return _resolution->height;
// }

// bool Display::IsPortrait() {
//   if (!_resolution) {
//     FLOG_ERROR("Resolution not initialized! Call Display::Init() first.");
//     throw std::runtime_error("Resolution not initialized");
//   }
//   return _resolution->is_portrait;
// }

// bool Display::IsTall() {
//   if (!_resolution) {
//     FLOG_ERROR("Resolution not initialized! Call Display::Init() first.");
//     throw std::runtime_error("Resolution not initialized");
//   }
//   if (_resolution->height >= 300) return true;
//   return false;
// }

// bool Display::IsWide() {
//   if (!_resolution) {
//     FLOG_ERROR("Resolution not initialized! Call Display::Init() first.");
//     throw std::runtime_error("Resolution not initialized");
//   }
//   if (_resolution->width >= 400) return true;
//   return false;
// }

void Display::LvglTickCallback(void* arg) { lv_tick_inc(kLvglTickPeriodMs); }

void Display::LvglPortTask(void* arg) {
  FLOG_INFO("Starting LVGL port task");

  esp_task_wdt_add(NULL);
  // Configure WDT to log errors instead of panicking
  // FIXME: This doesn't actually trigger at all
  // esp_task_wdt_config_t twdt_config = {
  //     .timeout_ms = 4000,   // 10 second timeout
  //     .idle_core_mask = 0,   // Don't monitor idle tasks
  //     .trigger_panic = false // Don't crash on timeout - just log!
  // };
  // esp_task_wdt_reconfigure(&twdt_config);

  uint32_t time_till_next_ms = 0;
  [[maybe_unused]] uint64_t start;  // for log printing
  while (1) {
    start = esp_timer_get_time();
    // Feed watchdog BEFORE potentially long LVGL operations
    esp_task_wdt_reset();
    {
      // std::lock_guard<std::mutex> lock(Display::GetLvglMutex());
      lv_obj_t* active_screen = lv_display_get_screen_active(Display::GetDisplayPtr());
      if (active_screen) {
        time_till_next_ms = lv_timer_handler();
        // Reset watchdog after LVGL processing
        esp_task_wdt_reset();
      } else {
        time_till_next_ms = 100;
        static uint32_t no_screen_count = 0;
        if (++no_screen_count % 50 == 0) {  // Every 5 seconds
          FLOG_ERROR("Waiting for active screen... (%u)", no_screen_count);
        }
      }
    }
    // LV_LOG_USER("LVGL handler time: %lli us", (esp_timer_get_time() - start));

    // PS_PUB_INT("heartbeat.ui", esp_timer_get_time()); //TODO: put a hearbeat something

    // Clamp delay values
    time_till_next_ms = std::max(time_till_next_ms, kLvglTaskMinDelayMs);
    time_till_next_ms = std::min(time_till_next_ms, kLvglTaskMaxDelayMs);

    // Use proper FreeRTOS delay with calculated time
    vTaskDelay(pdMS_TO_TICKS(time_till_next_ms));
  }
  FLOG_ERROR("LVGL port task exited unexpectedly");
  vTaskDelete(NULL);
}

}  // namespace toothless
