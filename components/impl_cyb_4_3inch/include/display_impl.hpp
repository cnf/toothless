#pragma once

#include <esp_lcd_panel_io.h>
#include <esp_lcd_panel_rgb.h>
#include <esp_lcd_touch.h>
#include <hal/spi_types.h>
#include <lvgl.h>

#include <memory>
// #include <mutex>

#define DISPLAY_BL_ON_LEVEL 1

namespace display {
namespace impl {

static constexpr size_t kLcdHRes = CONFIG_IMPL_CYB_4_3_INCH_HRES;  //<! horizontal resolution
static constexpr size_t kLcdVRes = CONFIG_IMPL_CYB_4_3_INCH_VRES;  //<! vertical resolution
static constexpr size_t kLcdDPI = 217;                             //<! display dpi

static constexpr unsigned int kLcdPixelClockHz = 18 * 1000 * 1000;  //<! pixel clock frequency for lcd
static constexpr uint8_t kLcdPixelClockPin = 42;                    /* PCLK */
static constexpr uint8_t kLcdDePin = 40;                            /* DE */
static constexpr uint8_t kLcdVSyncPin = 41;                         /* VSYNC */
static constexpr uint8_t kLcdHSyncPin = 39;                         /* HSYNC */
static constexpr uint8_t kLcdRed0Pin = 45;                          /* R0 */
static constexpr uint8_t kLcdRed1Pin = 48;                          /* R1 */
static constexpr uint8_t kLcdRed2Pin = 47;                          /* R2 */
static constexpr uint8_t kLcdRed3Pin = 21;                          /* R3 */
static constexpr uint8_t kLcdRed4Pin = 14;                          /* R4 */
static constexpr uint8_t kLcdGreen0Pin = 5;                         /* G0 */
static constexpr uint8_t kLcdGreen1Pin = 6;                         /* G1 */
static constexpr uint8_t kLcdGreen2Pin = 7;                         /* G2 */
static constexpr uint8_t kLcdGreen3Pin = 15;                        /* G3 */
static constexpr uint8_t kLcdGreen4Pin = 16;                        /* G4 */
static constexpr uint8_t kLcdGreen5Pin = 4;                         /* G5 */
static constexpr uint8_t kLcdBlue0Pin = 8;                          /* B0 */
static constexpr uint8_t kLcdBlue1Pin = 3;                          /* B1 */
static constexpr uint8_t kLcdBlue2Pin = 46;                         /* B2 */
static constexpr uint8_t kLcdBlue3Pin = 9;                          /* B3 */
static constexpr uint8_t kLcdBlue4Pin = 1;                          /* B4 */

static constexpr gpio_num_t kLcdBacklightPin = GPIO_NUM_2;  //<! lcd backlight pin

static constexpr uint8_t kLcdHSyncPolarity = 0;            /* hsync_polarity */
static constexpr uint8_t kLcdHSyncFrontPorch = 8;          /* hsync_front_porch */
static constexpr uint8_t kLcdHSyncPulseWidth = 4;          /* hsync_pulse_width */
static constexpr uint8_t kLcdHSyncBackPorch = 8;           /* hsync_back_porch */
static constexpr uint8_t kLcdVSyncPolarity = 0;            /* vsync_polarity */
static constexpr uint8_t kLcdVSyncFrontPorch = 8;          /* vsync_front_porch */
static constexpr uint8_t kLcdVSyncPulseWidth = 4;          /* vsync_pulse_width */
static constexpr uint8_t kLcdVSyncBackPorch = 8;           /* vsync_back_porch */
static constexpr uint8_t kLcdPixelClockActiveNegative = 1; /* pclk_active_neg */

static constexpr gpio_num_t kTouchI2cResetPin = GPIO_NUM_38;  //<! i2c reset pin for touch controller
static constexpr uint8_t kTouchI2cAddress = 0x5D;             //<! i2c address for touch controller

#define TOUCH_H_RES_MIN 0
#define TOUCH_H_RES_MAX 475
#define TOUCH_V_RES_MIN 0
#define TOUCH_V_RES_MAX 271

// static constexpr uint16_t kLcdColorDepth = LV_COLOR_FORMAT_RGB565;  //<! color depth used in the lcd panel
static constexpr uint8_t kLvglDrawBufferLines = 96;  // number of display lines in each draw buffer
static constexpr size_t kDrawBufferSize = CONFIG_IMPL_CYB_4_3_INCH_HRES * kLvglDrawBufferLines * sizeof(lv_color_t);
// static constexpr size_t kFullBufferSize =
//     CONFIG_IMPL_CYB_4_3_INCH_HRES * CONFIG_IMPL_CYB_4_3_INCH_VRES * sizeof(lv_color_t);

esp_err_t DisplayPanelSetup();
esp_err_t TouchPanelSetup();
inline esp_err_t BacklightSetup() {
  Backlight();
  return ESP_OK;
};
inline esp_err_t SetBrightness(uint8_t brightness) { return ESP_OK; };
void GetDisplayDimensions(uint16_t& width, uint16_t& height);
lv_display_t* GetDisplayObjPtr();

esp_err_t SetupQSPI();

void LvglFlushCallback(lv_display_t* disp, const lv_area_t* area, uint8_t* px_map);

void TouchMapCoordinates(esp_lcd_touch_handle_t tp, uint16_t* x, uint16_t* y, uint16_t* strength, uint8_t* point_num,
                         uint8_t max_point_num);

bool LvglFlushReadyCallback(esp_lcd_panel_handle_t panel, const esp_lcd_rgb_panel_event_data_t* edata, void* user_ctx);

void LvglTouchCallback(lv_indev_t* indev, lv_indev_data_t* data);

void TurnOn();

void ShowBootScreen();

void Backlight();

void BacklightTimerCallback(void* arg);

}  // namespace impl
}  // namespace display
