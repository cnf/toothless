// cSpell: words lvgl
#include "config.h"

#include "funlog.h"
#include "lvgl_port.hpp"
#include "ui/display/display.hpp"
#include "ui/display/lvgl_port.hpp"
#include <esp_lcd_panel_ops.h>
#include <esp_lcd_touch_xpt2046.h>
#include <esp_task_wdt.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <sys/param.h>
#include <unistd.h>

// #include "esp_timer.h"
// #include "freertos/FreeRTOS.h"
// #include "freertos/task.h"
// #include <sys/lock.h>
// #include <esp_lcd_panel_vendor.h>
// #include <esp_lcd_panel_io.h>

// #include <esp_timer.h>

namespace toothless {
namespace callback {

#define LVGL_TASK_MAX_DELAY_MS 500
#define LVGL_TASK_MIN_DELAY_MS 1000 / CONFIG_FREERTOS_HZ

/// @brief LVGL callback to flush a portion of the display
/// @param disp
/// @param area
/// @param px_map
void lvgl_flush_cb(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map) {
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
}

/// @brief LVGL callback to increase the tick count
/// @param arg
void lvgl_increase_tick(void *arg) {
  /* Tell LVGL how many milliseconds has elapsed */
  lv_tick_inc(LVGL_TICK_PERIOD_MS);
}

/// @brief LVGL callback to notify actions are done, and a flush can be done
/// @param panel_io
/// @param edata
/// @param user_ctx
/// @return
bool lvgl_notify_flush_ready(esp_lcd_panel_io_handle_t panel_io, esp_lcd_panel_io_event_data_t *edata, void *user_ctx) {
  lv_display_t *disp = (lv_display_t *)user_ctx;
  lv_display_flush_ready(disp);
  return false;
}

/// @brief  LVGL task to handle LVGL timers and events
/// @param arg
void lvgl_port_task(void *arg) {
  FLOG_INFO("Starting LVGL port task");
  // Display *display = static_cast<Display *>(arg);
  // std::shared_ptr<lv_display_t> = static_cast<lv_display_t *>(arg);
  // lv_display_t *display = Display::GetDisplayPtr();

  esp_task_wdt_add(NULL);
  vTaskDelay(pdMS_TO_TICKS(500)); // Wait 500ms for UI setup

  uint32_t time_till_next_ms = 0;
  while (1) {
    {
      std::lock_guard<std::mutex> lock(Display::GetLvglMutex());
      lv_obj_t *active_screen = lv_display_get_screen_active(Display::GetDisplayPtr());
      if (active_screen) {
        time_till_next_ms = lv_timer_handler();
      } else {
        time_till_next_ms = 100;
        static uint32_t no_screen_count = 0;
        if (++no_screen_count % 50 == 0) { // Every 5 seconds
          FLOG_DEBUG("Waiting for active screen... (%u)", no_screen_count);
        }
      }
    }

    // Feed the watchdog to prevent timeout
    esp_task_wdt_reset();

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
void lvgl_touch_cb(lv_indev_t *indev, lv_indev_data_t *data) {
  // TODO: not sure how this works
  // uint16_t touchpad_x[1] = {0};
  // uint16_t touchpad_y[1] = {0};
  // uint8_t touchpad_cnt = 0;

  // esp_lcd_touch_handle_t *touch_pad = lv_indev_get_user_data(indev);
  // esp_lcd_touch_read_data(touch_pad);
  // /* Get coordinates */
  // bool touchpad_pressed = esp_lcd_touch_get_coordinates(touch_pad, touchpad_x, touchpad_y, NULL, &touchpad_cnt, 1);

  // if (touchpad_pressed && touchpad_cnt > 0) {
  //   data->point.x = touchpad_x[0];
  //   data->point.y = touchpad_y[0];
  //   data->state = LV_INDEV_STATE_PRESSED;
  // } else {
  //   data->state = LV_INDEV_STATE_RELEASED;
  // }
  return;
}

} // namespace callback
} // namespace toothless