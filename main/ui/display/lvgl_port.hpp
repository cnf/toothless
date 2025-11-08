#pragma once

#include <esp_lcd_panel_io.h>
#include <lvgl.h>
#include <mutex>

namespace toothless {

/*
Top-left: x=425, y=285
Bottom-right: x=63, y=24
*/

// Touch calibration constants - based on actual measurements
#define RAW_X_MIN 45  // Raw touch value at RIGHT edge (inverted)
#define RAW_X_MAX 445 // Raw touch value at LEFT edge (inverted)
#define RAW_Y_MIN 30  // Raw touch value at BOTTOM edge (inverted)
#define RAW_Y_MAX 285 // Raw touch value at TOP edge (inverted)

#define LVGL_TASK_MAX_DELAY_MS 500
#define LVGL_TASK_MIN_DELAY_MS 1000 / CONFIG_FREERTOS_HZ

// Forward declaration
class Display;

namespace callback {

void lvgl_flush_cb(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map);

void lvgl_increase_tick(void *arg);

bool lvgl_notify_flush_ready(esp_lcd_panel_io_handle_t panel_io, esp_lcd_panel_io_event_data_t *edata, void *user_ctx);

void lvgl_port_task(void *arg);

void lvgl_touch_cb(lv_indev_t *indev, lv_indev_data_t *data);

void lvgl_display_event_cb(lv_event_t *e);

void lvgl_boot_screen();

void backlight_timer_cb(void *arg);

} // namespace callback
} // namespace toothless