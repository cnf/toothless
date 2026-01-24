#pragma once

#include <esp_lcd_panel_io.h>
#include <hal/spi_types.h>
#include <lvgl.h>

#include <memory>

#include "esp_lcd_panel_rgb.h"
#include "impl_config.hpp"

namespace display {
namespace impl {

// static constexpr size_t kHRes = CONFIG_IMPL_LILYGO_T_HMI_HRES;  //<! horizontal resolution
// static constexpr size_t kVRes = CONFIG_IMPL_LILYGO_T_HMI_VRES;  //<! vertical resolution
static constexpr size_t kLcdDPI = 217;  //<! display dpi

static constexpr unsigned int kPixelClockHz = 10 * 1000 * 1000;  //<! pixel clock frequency for lcd
static constexpr gpio_num_t kTftBacklightPin = GPIO_NUM_38;      //<! lcd backlight pin
static constexpr gpio_num_t kTftData0Pin = GPIO_NUM_48;          //<! lcd data0 pin
static constexpr gpio_num_t kTftData1Pin = GPIO_NUM_47;          //<! lcd data1 pin
static constexpr gpio_num_t kTftData2Pin = GPIO_NUM_39;          //<! lcd data2 pin
static constexpr gpio_num_t kTftData3Pin = GPIO_NUM_40;          //<! lcd data3 pin
static constexpr gpio_num_t kTftData4Pin = GPIO_NUM_41;          //<! lcd data4 pin
static constexpr gpio_num_t kTftData5Pin = GPIO_NUM_42;          //<! lcd data5 pin
static constexpr gpio_num_t kTftData6Pin = GPIO_NUM_45;          //<! lcd data6 pin
static constexpr gpio_num_t kTftData7Pin = GPIO_NUM_46;          //<! lcd data7 pin
static constexpr gpio_num_t kTftResetPin = GPIO_NUM_NC;          //<! lcd reset pin
static constexpr gpio_num_t kTftCSPin = GPIO_NUM_6;              //<! lcd cs pin
static constexpr gpio_num_t kTftDCPin = GPIO_NUM_7;              //<! lcd dc pin
static constexpr gpio_num_t kTftWRPin = GPIO_NUM_8;              //<! lcd wr pin
static constexpr size_t kTftCmdBits = 8;                         //<! lcd command bits
static constexpr size_t kTftParamBits = 8;                       //<! lcd data bits

static constexpr gpio_num_t kTouchMisoPin = GPIO_NUM_4;  //<! touch miso pin/
static constexpr gpio_num_t kTouchMosiPin = GPIO_NUM_3;  //<! touch mosi pin
static constexpr gpio_num_t kTouchSclkPin = GPIO_NUM_1;  //<! touch sclk pin
static constexpr gpio_num_t kTouchCSPin = GPIO_NUM_2;    //<! touch cs pin
static constexpr gpio_num_t kTouchIntPin = GPIO_NUM_9;   //<! touch irq pin

static constexpr size_t kBytesPerPixel = sizeof(lv_color_t);  //<! (LV_COLOR_DEPTH / 8);
static constexpr uint16_t kLvglDrawBufferLines = 80;          //<! number of display lines in each draw buffer
static constexpr uint32_t kDisplayBufferPixels = kHRes * kLvglDrawBufferLines;  //<! size of each draw buffer in pixels
static constexpr size_t kDisplayBufferBytes = kDisplayBufferPixels * kBytesPerPixel;

esp_err_t DisplayPanelSetup();
esp_err_t TouchPanelSetup();
inline esp_err_t BacklightSetup() {
  Backligjht();
  return ESP_OK;
};
inline esp_err_t SetBrightness(uint8_t brightness) { return ESP_OK; };
void GetDisplayDimensions(uint16_t& width, uint16_t& height);
lv_display_t* GetDisplayObjPtr();

esp_err_t SetupSPI();

void LvglFlushCallback(lv_display_t* disp, const lv_area_t* area, uint8_t* px_map);

bool LvglFlushReadyCallback(esp_lcd_panel_io_handle_t panel, esp_lcd_panel_io_event_data_t* edata, void* user_ctx);

void LvglTouchCallback(lv_indev_t* indev, lv_indev_data_t* data);

void TurnOn();

void ShowBootScreen();

void Backlight();

void BacklightTimerCallback(void* arg);

}  // namespace impl
}  // namespace display
