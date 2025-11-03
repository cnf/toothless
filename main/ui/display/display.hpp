#pragma once

#include <driver/spi_master.h>
#include <hal/spi_types.h>
#include <lvgl.h>
#include <memory>
#include <mutex>

extern "C" {
#include <pubsub.h>
}

#define PORTRAIT true

#define LVGL_DRAW_BUF_LINES 20 // number of display lines in each draw buffer
#define LVGL_TASK_STACK_SIZE (16 * 1024)
#define LVGL_TASK_PRIORITY 2
#define LCD_PIXEL_CLOCK_HZ (20 * 1000 * 1000)
// Bit number used to represent command and parameter
#define LCD_CMD_BITS 8
#define LCD_PARAM_BITS 8

#define DISPLAY_BL_ON_LEVEL 1

namespace toothless {
class Display {
public:
  static esp_err_t Init();
  static esp_err_t SetupPanel();
  static esp_err_t SetupTouchPanel();

  /// @brief Get the mutex for LVGL synchronization
  static std::mutex &GetLvglMutex();

  /// @brief Get the display pointer for callbacks
  static lv_display_t *GetDisplayPtr();
  // static void Cleanup() {
  //   if (_draw_buf1) {
  //     free(_draw_buf1);
  //     _draw_buf1 = nullptr;
  //   }
  //   if (_draw_buf2) {
  //     free(_draw_buf2);
  //     _draw_buf2 = nullptr;
  //   }
  // }

private:
  // static void *_draw_buf1;
  // static void *_draw_buf2;
  // static spi_host_device_t _spi_host;
};
} // namespace toothless
