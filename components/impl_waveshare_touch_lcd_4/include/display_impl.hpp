#pragma once

#include <esp_lcd_panel_io.h>
#include <esp_lcd_panel_rgb.h>
#include <esp_lcd_touch.h>
#include <hal/spi_types.h>
#include <lvgl.h>

#include <memory>

#include "esp_lcd_panel_rgb.h"
#include "implementation.hpp"

namespace impl {
namespace display {

/* Display Pinout for Waveshare Touch LCD 4
#define BSP_LCD_VSYNC (GPIO_NUM_39)
#define BSP_LCD_HSYNC (GPIO_NUM_38)
#define BSP_LCD_DE (GPIO_NUM_40)
#define BSP_LCD_PCLK (GPIO_NUM_41)
#define BSP_LCD_DISP (GPIO_NUM_NC)
#define BSP_LCD_DATA0 (GPIO_NUM_5)
#define BSP_LCD_DATA1 (GPIO_NUM_45)
#define BSP_LCD_DATA2 (GPIO_NUM_48)
#define BSP_LCD_DATA3 (GPIO_NUM_47)
#define BSP_LCD_DATA4 (GPIO_NUM_21)
#define BSP_LCD_DATA5 (GPIO_NUM_14)
#define BSP_LCD_DATA6 (GPIO_NUM_13)
#define BSP_LCD_DATA7 (GPIO_NUM_12)
#define BSP_LCD_DATA8 (GPIO_NUM_11)
#define BSP_LCD_DATA9 (GPIO_NUM_10)
#define BSP_LCD_DATA10 (GPIO_NUM_9)
#define BSP_LCD_DATA11 (GPIO_NUM_46)
#define BSP_LCD_DATA12 (GPIO_NUM_3)
#define BSP_LCD_DATA13 (GPIO_NUM_8)
#define BSP_LCD_DATA14 (GPIO_NUM_18)
#define BSP_LCD_DATA15 (GPIO_NUM_17)

#define BSP_LCD_IO_SPI_CS (GPIO_NUM_42)
#define BSP_LCD_IO_SPI_SCL (GPIO_NUM_2)
#define BSP_LCD_IO_SPI_SDA (GPIO_NUM_1)

#define BSP_LCD_BACKLIGHT (GPIO_NUM_NC)
#define EXP_LCD_RST (IO_EXPANDER_PIN_NUM_3)
#define EXP_LCD_TOUCH_RST (IO_EXPANDER_PIN_NUM_1)
#define BSP_LCD_TOUCH_INT (GPIO_NUM_NC)

#define BSP_BEE_EN (IO_EXPANDER_PIN_NUM_6)
#define EXP_SYS_EN (IO_EXPANDER_PIN_NUM_5)
#define EXP_RTC_INT (IO_EXPANDER_PIN_NUM_7)
*/

// static constexpr size_t kHRes = CONFIG_IMPL_LILYGO_T_HMI_HRES;  //<! horizontal resolution
// static constexpr size_t kVRes = CONFIG_IMPL_LILYGO_T_HMI_VRES;  //<! vertical resolution
// static constexpr size_t kLcdDPI = 217;  //<! display dpi

#define BSP_LCD_BITS_PER_PIXEL (16)
#define BSP_LCD_BIT_PER_PIXEL (18)
#define BSP_RGB_DATA_WIDTH (16)

#define TOUCH_H_RES_MIN 0
#define TOUCH_H_RES_MAX 480
#define TOUCH_V_RES_MIN 0
#define TOUCH_V_RES_MAX 480

static constexpr unsigned int kPixelClockHz = 16 * 1000 * 1000;  //<! pixel clock frequency for lcd

static constexpr gpio_num_t kLcdVSPin = GPIO_NUM_39;    //<! lcd vsync pin
static constexpr gpio_num_t kLcdHSPin = GPIO_NUM_38;    //<! lcd hsync pin
static constexpr gpio_num_t kLcdDEPin = GPIO_NUM_40;    //<! lcd de pin
static constexpr gpio_num_t kLcdPCLKPin = GPIO_NUM_41;  //<! lcd pclk pin

static constexpr gpio_num_t kLcdData0Pin = GPIO_NUM_5;    //<! lcd data0 pin
static constexpr gpio_num_t kLcdData1Pin = GPIO_NUM_45;   //<! lcd data1 pin
static constexpr gpio_num_t kLcdData2Pin = GPIO_NUM_48;   //<! lcd data2 pin
static constexpr gpio_num_t kLcdData3Pin = GPIO_NUM_47;   //<! lcd data3 pin
static constexpr gpio_num_t kLcdData4Pin = GPIO_NUM_21;   //<! lcd data4 pin
static constexpr gpio_num_t kLcdData5Pin = GPIO_NUM_14;   //<! lcd data5 pin
static constexpr gpio_num_t kLcdData6Pin = GPIO_NUM_13;   //<! lcd data6 pin
static constexpr gpio_num_t kLcdData7Pin = GPIO_NUM_12;   //<! lcd data7 pin
static constexpr gpio_num_t kLcdData8Pin = GPIO_NUM_11;   //<! lcd data8 pin
static constexpr gpio_num_t kLcdData9Pin = GPIO_NUM_10;   //<! lcd data9 pin
static constexpr gpio_num_t kLcdData10Pin = GPIO_NUM_9;   //<! lcd data10 pin
static constexpr gpio_num_t kLcdData11Pin = GPIO_NUM_46;  //<! lcd data11 pin
static constexpr gpio_num_t kLcdData12Pin = GPIO_NUM_3;   //<! lcd data12 pin
static constexpr gpio_num_t kLcdData13Pin = GPIO_NUM_8;   //<! lcd data13 pin
static constexpr gpio_num_t kLcdData14Pin = GPIO_NUM_18;  //<! lcd data14 pin
static constexpr gpio_num_t kLcdData15Pin = GPIO_NUM_17;  //<! lcd data15 pin

static constexpr gpio_num_t kLcdSPICSPin = GPIO_NUM_42;   //<! lcd spi cs pin
static constexpr gpio_num_t kLcdSPISclkPin = GPIO_NUM_2;  //<! lcd spi sclk pin
static constexpr gpio_num_t kLcdSPISdaPin = GPIO_NUM_1;   //<! lcd spi sda pin

static constexpr size_t kLcdDataWidth = 16;     //<! lcd data width
static constexpr size_t kLcdBitsPerPixel = 16;  //<! lcd bits per pixel

static constexpr gpio_num_t kLcdBacklightPin = GPIO_NUM_NC;  //<! lcd backlight pin
// static constexpr gpio_num_t kLcdResetPin = GPIO_NUM_NC;      //<! lcd reset pin

static constexpr gpio_num_t kTouchI2cResetPin =
    GPIO_NUM_NC;                                   // IO_EXPANDER_PIN_NUM_1;  //<! i2c reset pin for touch controller
static constexpr uint8_t kTouchI2cAddress = 0x5D;  //<! i2c address for touch controller

static constexpr size_t kLcdRgbBufferCount = 2;  //<! number of rgb frame buffers

static constexpr size_t kBytesPerPixel = sizeof(lv_color_t);  //<! (LV_COLOR_DEPTH / 8);
static constexpr uint16_t kLvglDrawBufferLines = 96;          // 96;          //<! number of display lines in each draw
                                                              // buffer
static constexpr uint32_t kDisplayBufferPixels = kHRes * kLvglDrawBufferLines;  //<! size of each draw buffer in pixels
static constexpr size_t kDisplayBufferBytes = kDisplayBufferPixels * kBytesPerPixel;
static constexpr size_t kBounceBuffer = kHRes * (kVRes / 40);  //<! size of bounce buffer in pixels

// Ensure bounce buffer divides frame buffer evenly
static_assert(kDisplayBufferPixels % kBounceBuffer == 0, "Frame buffer size must be a multiple of bounce buffer size");

esp_err_t DisplayPanelSetup();
esp_err_t TouchPanelSetup();
esp_err_t BacklightSetup();
esp_err_t SetBrightness(uint8_t brightness);

void GetDisplayDimensions(uint16_t& width, uint16_t& height);
lv_display_t* GetDisplayObjPtr();

esp_err_t SetupSPI();

void LvglFlushCallback(lv_display_t* disp, const lv_area_t* area, uint8_t* px_map);

void TouchMapCoordinates(esp_lcd_touch_handle_t tp, uint16_t* x, uint16_t* y, uint16_t* strength, uint8_t* point_num,
                         uint8_t max_point_num);

bool LvglFlushReadyCallback(esp_lcd_panel_io_handle_t panel, esp_lcd_panel_io_event_data_t* edata, void* user_ctx);

void LvglTouchCallback(lv_indev_t* indev, lv_indev_data_t* data);

void TurnOn();

void ShowBootScreen();

void Backlight();

void BacklightTimerCallback(void* arg);

}  // namespace display
}  // namespace impl
