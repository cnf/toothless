#pragma once

#include <esp_lcd_panel_io.h>
#include <lvgl.h>
#include <mutex>

namespace toothless {

// Forward declaration
class Display;

namespace callback {

void lvgl_flush_cb(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map);

void lvgl_increase_tick(void *arg);

bool lvgl_notify_flush_ready(esp_lcd_panel_io_handle_t panel_io, esp_lcd_panel_io_event_data_t *edata, void *user_ctx);

void lvgl_port_task(void *arg);

void lvgl_touch_cb(lv_indev_t *indev, lv_indev_data_t *data);

} // namespace callback
} // namespace toothless