#pragma once

#include <esp_lcd_panel_io.h>
#include <hal/spi_types.h>
#include <lvgl.h>

#include <memory>

namespace display {
namespace impl {

static constexpr size_t kHRes = CONFIG_IMPL_M5BASIC_HRES;  //<! horizontal resolution
static constexpr size_t kVRes = CONFIG_IMPL_M5BASIC_VRES;  //<! vertical resolution
// static constexpr size_t kLcdDPI = 217;  //<! display dpi

/*
#define BSP_LCD_MOSI          (GPIO_NUM_5)
#define BSP_LCD_MISO          (GPIO_NUM_NC)
#define BSP_LCD_PCLK          (GPIO_NUM_6)
#define BSP_LCD_CS            (GPIO_NUM_7)
#define BSP_LCD_DC            (GPIO_NUM_4)
#define BSP_LCD_RST           (GPIO_NUM_8)
#define BSP_LCD_BACKLIGHT     (GPIO_NUM_9)
#define BSP_LCD_TOUCH_INT     (GPIO_NUM_14)
*/

static constexpr unsigned int kPixelClockHz = 40 * 1000 * 1000;  //<! pixel clock frequency for lcd
static constexpr size_t kLcdCmdBits = 8;                         //<! lcd command bits
static constexpr size_t kLcdParamBits = 8;                       //<! lcd data bits

// static constexpr gpio_num_t kLcdDCPin = GPIO_NUM_4;         //<! lcd Data/Command pin
// static constexpr gpio_num_t kLcdMosiPin = GPIO_NUM_5;       //<! lcd mosi pin
// static constexpr gpio_num_t kLcdPclkPin = GPIO_NUM_6;       //<! lcd sclk pin
// static constexpr gpio_num_t kLcdCSPin = GPIO_NUM_7;         //<! lcd cs pin
// static constexpr gpio_num_t kLcdResetPin = GPIO_NUM_8;      //<! lcd reset pin
// static constexpr gpio_num_t kLcdBackLightPin = GPIO_NUM_9;  //<! lcd backlight pin
// static constexpr gpio_num_t kTouchIntPin = GPIO_NUM_14;     //<! touch irq pin
// static constexpr uint8_t kTouchI2cAddress = 0x38;           //<! i2c address for touch controller

// static constexpr gpio_num_t kEncoderAPin = GPIO_NUM_41;    //<! encoder A pin
// static constexpr gpio_num_t kEncoderBPin = GPIO_NUM_40;    //<! encoder B pin
// static constexpr gpio_num_t kEncoderBtnPin = GPIO_NUM_42;  //<! encoder button pin

static constexpr size_t kBytesPerPixel = sizeof(lv_color_t);  //<! (LV_COLOR_DEPTH / 8);
static constexpr uint16_t kDrawBufferLines = 80;              //<! number of display lines in each draw buffer
static constexpr uint32_t kDrawBufferPixels = kHRes * kDrawBufferLines;  //<! size of each draw buffer in pixels
static constexpr size_t kDrawBufferSize = kDrawBufferPixels * kBytesPerPixel;

static constexpr size_t kMaxTransferSize = kDrawBufferSize + 8;  //<! max spi transfer size

esp_err_t DisplayPanelSetup();
esp_err_t TouchPanelSetup();
inline esp_err_t BacklightSetup() {
  Backligjht();
  return ESP_OK;
};
inline esp_err_t SetBrightness(uint8_t brightness) { return ESP_OK; };
esp_err_t EncoderSetup();
void GetDisplayDimensions(uint16_t& width, uint16_t& height);
lv_display_t* GetDisplayObjPtr();

esp_err_t SetupSPI();

esp_err_t LvgLBufferSetupPartial();

void LvglFlushCallback(lv_display_t* disp, const lv_area_t* area, uint8_t* px_map);

bool LvglFlushReadyCallback(esp_lcd_panel_io_handle_t panel, esp_lcd_panel_io_event_data_t* edata, void* user_ctx);

void LvglTouchCallback(lv_indev_t* indev, lv_indev_data_t* data);

void LvglEncoderCallback(lv_indev_t* indev, lv_indev_data_t* data);

void TurnOn();

void ShowBootScreen();

void Backlight();

void BacklightTimerCallback(void* arg);

}  // namespace impl
}  // namespace display
