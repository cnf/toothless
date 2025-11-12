#pragma once

#include <driver/spi_master.h>
#include <esp_lcd_panel_io.h>
#include <hal/spi_types.h>
#include <lvgl.h>

#include <memory>
#include <mutex>

// extern "C" {
// #include <pubsub.h>
// }

// #define PORTRAIT false

// #define LVGL_DRAW_BUF_LINES 40  // number of display lines in each draw buffer
// #define LVGL_TASK_STACK_SIZE (16 * 1024)
// #define LVGL_TASK_PRIORITY 2
// #define LCD_PIXEL_CLOCK_HZ (20 * 1000 * 1000)
// // Bit number used to represent command and parameter
// #define LCD_CMD_BITS 8
// #define LCD_PARAM_BITS 8

#define DISPLAY_BL_ON_LEVEL 1

namespace display {
namespace impl {

static constexpr unsigned int kLcdPixelClockHz = 20 * 1000 * 1000;  //<! spi clock frequency for lcd
static constexpr uint8_t kLcdCmdBits = 8;                           //<! bit number used to represent command
static constexpr uint8_t kLcdParamBits = 8;                         //<! bit number used to represent parameter
static constexpr uint8_t kLvglDrawBufferLines = 40;                 // number of display lines in each draw buffer

esp_err_t DisplayPanelSetup();
esp_err_t TouchPanelSetup();
void GetDisplayDimensions(uint16_t& width, uint16_t& height);
lv_display_t* GetDisplayObjPtr();

void LvglFlushCallback(lv_display_t* disp, const lv_area_t* area, uint8_t* px_map);

bool LvglFlushReadyCallback(esp_lcd_panel_io_handle_t io, esp_lcd_panel_io_event_data_t* edata, void* user_data);

void LvglTouchCallback(lv_indev_t* indev, lv_indev_data_t* data);

void TurnOn();

void ShowBootScreen();

void Backlight();

void BacklightTimerCallback(void* arg);

}  // namespace impl
}  // namespace display
