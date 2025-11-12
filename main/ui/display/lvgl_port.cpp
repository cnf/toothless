// cSpell: words lvgl
#include "lvgl_port.hpp"

#include <esp_lcd_panel_ops.h>
#include <esp_lcd_touch_xpt2046.h>
#include <esp_task_wdt.h>
#include <esp_timer.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <sys/param.h>
#include <unistd.h>

#include "config.h"
#include "funlog.h"
#include "ui/display/display.hpp"
#include "ui/display/lvgl_port.hpp"

extern "C" {
#include <pubsub.h>
}

namespace toothless {
namespace callback {

// // Utility functions for touch calibration
// static int32_t map(int32_t value, int32_t in_min, int32_t in_max, int32_t out_min, int32_t out_max) {
//   return (value - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
// }

// static int32_t constrain(int32_t value, int32_t min_val, int32_t max_val) {
//   if (value < min_val)
//     return min_val;
//   if (value > max_val)
//     return max_val;
//   return value;
// }

/// @brief LVGL callback to flush a portion of the display
/// @param disp
/// @param area
/// @param px_map
void lvgl_flush_cb(lv_display_t* disp, const lv_area_t* area, uint8_t* px_map) {
  uint32_t starter = esp_timer_get_time();

  esp_lcd_panel_handle_t panel_handle = (esp_lcd_panel_handle_t)lv_display_get_user_data(disp);
  int offsetx1 = area->x1;
  int offsetx2 = area->x2;
  int offsety1 = area->y1;
  int offsety2 = area->y2;

  // because SPI LCD is big-endian, we need to swap the RGB bytes order
  lv_draw_sw_rgb565_swap(px_map, (offsetx2 + 1 - offsetx1) * (offsety2 + 1 - offsety1));

  // copy a buffer's content to a specific area of the display
  esp_lcd_panel_draw_bitmap(panel_handle, offsetx1, offsety1, offsetx2 + 1, offsety2 + 1, px_map);

  // CRITICAL: Tell LVGL that flushing is done
  lv_display_flush_ready(disp);
  LV_LOG_INFO("LVGL flush time: %lli us", (esp_timer_get_time() - starter));
}

/// @brief LVGL callback to increase the tick count
/// @param arg
void lvgl_increase_tick(void* arg) {
  /* Tell LVGL how many milliseconds has elapsed */
  lv_tick_inc(LVGL_TICK_PERIOD_MS);
}

/// @brief LVGL callback to notify actions are done, and a flush can be done
/// @param panel_io
/// @param edata
/// @param user_ctx
/// @return
bool lvgl_notify_flush_ready(esp_lcd_panel_io_handle_t panel_io, esp_lcd_panel_io_event_data_t* edata, void* user_ctx) {
  lv_display_t* disp = (lv_display_t*)user_ctx;
  lv_display_flush_ready(disp);
  return false;
}

/// @brief  LVGL task to handle LVGL timers and events
/// @param arg
void lvgl_port_task(void* arg) {
  FLOG_INFO("Starting LVGL port task");

  esp_task_wdt_add(NULL);
  // Configure WDT to log errors instead of panicking
  // BUG: This doesn't actually trigger at all
  // esp_task_wdt_config_t twdt_config = {
  //     .timeout_ms = 4000,   // 10 second timeout
  //     .idle_core_mask = 0,   // Don't monitor idle tasks
  //     .trigger_panic = false // Don't crash on timeout - just log!
  // };
  // esp_task_wdt_reconfigure(&twdt_config);
  // vTaskDelay(pdMS_TO_TICKS(500)); // Wait 500ms for UI setup

  uint32_t time_till_next_ms = 0;
  while (1) {
    uint64_t start = esp_timer_get_time();
    // Feed watchdog BEFORE potentially long LVGL operations
    esp_task_wdt_reset();

    {
      std::lock_guard<std::mutex> lock(Display::GetLvglMutex());
      lv_obj_t* active_screen = lv_display_get_screen_active(Display::GetDisplayPtr());
      if (active_screen) {
        time_till_next_ms = lv_timer_handler();
        // Reset watchdog after LVGL processing
        esp_task_wdt_reset();
      } else {
        time_till_next_ms = 100;
        static uint32_t no_screen_count = 0;
        if (++no_screen_count % 50 == 0) {  // Every 5 seconds
          FLOG_DEBUG("Waiting for active screen... (%u)", no_screen_count);
        }
      }
    }
    LV_LOG_INFO("LVGL handler time: %lli us", (esp_timer_get_time() - start));

    // PS_PUB_INT("heartbeat.ui", esp_timer_get_time()); //TODO: put a hearbeat something

    // Clamp delay values
    time_till_next_ms = MAX(time_till_next_ms, LVGL_TASK_MIN_DELAY_MS);
    time_till_next_ms = MIN(time_till_next_ms, LVGL_TASK_MAX_DELAY_MS);

    // Use proper FreeRTOS delay with calculated time
    vTaskDelay(pdMS_TO_TICKS(time_till_next_ms));
  }
  FLOG_ERROR("LVGL port task exited unexpectedly");
  vTaskDelete(NULL);
}

/// @brief LVGL touch input device read callback
/// @param indev
/// @param data
void lvgl_touch_cb(lv_indev_t* indev, lv_indev_data_t* data) {
  uint16_t touchpad_x[1] = {0};
  uint16_t touchpad_y[1] = {0};
  uint8_t touchpad_cnt = 0;

  // Get the touch controller handle from LVGL input device user data
  esp_lcd_touch_handle_t touch_handle = (esp_lcd_touch_handle_t)lv_indev_get_user_data(indev);

  // Read current touch data from the controller
  esp_err_t ret = esp_lcd_touch_read_data(touch_handle);
  if (ret != ESP_OK) {
    // If read failed, report no touch
    data->state = LV_INDEV_STATE_RELEASED;
    return;
  }

  // Get coordinates and touch state
  bool touchpad_pressed = esp_lcd_touch_get_coordinates(touch_handle, touchpad_x, touchpad_y, NULL, &touchpad_cnt, 1);

  if (touchpad_pressed && touchpad_cnt > 0) {
    // // FLOG_INFO("Raw: x=%d, y=%d", touchpad_x[0], touchpad_y[0]);
    // // Touch detected - report coordinates
    // // Get raw coordinates first
    // uint16_t raw_x = touchpad_x[0];
    // uint16_t raw_y = touchpad_y[0];

    // // // Apply calibration mapping
    // data->point.x = map(raw_x, RAW_X_MIN, RAW_X_MAX, 0, 480);
    // data->point.y = map(raw_y, RAW_Y_MIN, RAW_Y_MAX, 0, 320);

    // // // Clamp to screen bounds
    // data->point.x = constrain(data->point.x, 0, 479);
    // data->point.y = constrain(data->point.y, 0, 319);
    data->point.x = touchpad_x[0];
    data->point.y = touchpad_y[0];
    data->state = LV_INDEV_STATE_PRESSED;

    // Optional: Add touch coordinate logging for debugging
    static uint32_t log_count = 0;
    if (++log_count % 10 == 0) {  // Log every 10th touch event
      FLOG_DEBUG("Touch: x=%d, y=%d", data->point.x, data->point.y);
    }
  } else {
    // No touch detected
    data->state = LV_INDEV_STATE_RELEASED;
  }
  return;
}

void lvgl_display_event_cb(lv_event_t* e) {
  // lv_event_code_t code = lv_event_get_code(e);
  // if (code == LV_EVENT_REFR_READY) {
  //   // First refresh done
  //   gpio_set_level(CONFIG_TL_DISPLAY_BACKLIGHT_PIN, 1);
  //   // Optionally remove this event callback after first call
  //   lv_event_clear(lv_event_get_target(e), LV_EVENT_REFR_READY, lvgl_display_event_cb);
  // }
}

void lvgl_boot_screen() {
  FLOG_INFO("Boot Screen");
  std::lock_guard<std::mutex> lock(Display::GetLvglMutex());

  lv_obj_t* boot_scr = lv_obj_create(NULL);
  lv_obj_set_style_bg_color(boot_scr, lv_color_black(), 0);
  lv_obj_set_style_bg_opa(boot_scr, LV_OPA_COVER, 0);

  // lv_obj_t* obj = lv_obj_create(boot_scr);
  // lv_obj_center(obj);

  lv_obj_t* label = lv_label_create(boot_scr);
  lv_label_set_text(label, "Toothless");
  lv_obj_set_style_text_font(label, &lv_font_montserrat_32, 0);
  lv_obj_center(label);

  lv_obj_t* sub_label = lv_label_create(boot_scr);
  lv_label_set_text(sub_label, "Initializing...");
  lv_obj_set_style_text_font(sub_label, &lv_font_montserrat_20, 0);
  lv_obj_align_to(sub_label, label, LV_ALIGN_OUT_BOTTOM_MID, 0, 10);

  lv_display_flush_ready(Display::GetDisplayPtr());

  lv_screen_load(boot_scr);
}

void backlight_timer_cb(void* arg) {
  ESP_ERROR_CHECK(gpio_set_direction((gpio_num_t)CONFIG_TL_DISPLAY_BACKLIGHT_PIN, GPIO_MODE_OUTPUT));
  gpio_set_level((gpio_num_t)CONFIG_TL_DISPLAY_BACKLIGHT_PIN, DISPLAY_BL_ON_LEVEL);
}

}  // namespace callback
}  // namespace toothless