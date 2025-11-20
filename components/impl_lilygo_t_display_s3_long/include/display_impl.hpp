#pragma once

#include <driver/spi_master.h>
#include <esp_lcd_panel_io.h>
#include <hal/spi_types.h>
#include <lvgl.h>

#include <memory>

namespace display {
namespace impl {

#define AXS_GET_POINT_NUM(buf) buf[1]
#define AXS_GET_GESTURE_TYPE(buf) buf[0]
#define AXS_GET_POINT_X(buf, point_index) \
  (((uint16_t)(buf[6 * point_index + 2] & 0x0F) << 8) + (uint16_t)buf[6 * point_index + 3])
#define AXS_GET_POINT_Y(buf, point_index) \
  (((uint16_t)(buf[6 * point_index + 4] & 0x0F) << 8) + (uint16_t)buf[6 * point_index + 5])
#define AXS_GET_POINT_EVENT(buf, point_index) (buf[6 * point_index + 2] >> 6)

static constexpr int32_t kHRes = 180;
static constexpr int32_t kVRes = 640;

static constexpr gpio_num_t kLcdBacklightPin = GPIO_NUM_1;  //<! lcd backlight pin
static constexpr gpio_num_t kLcdCsPin = GPIO_NUM_12;        //<! lcd chip select pin
static constexpr gpio_num_t kLcdResetPin = GPIO_NUM_16;     //<! lcd reset pin
static constexpr gpio_num_t kLcdTePin = GPIO_NUM_NC;        //<! lcd te pin
static constexpr gpio_num_t kLcdData0Pin = GPIO_NUM_13;     //<! lcd data0 pin (QSPI D0)
static constexpr gpio_num_t kLcdData1Pin = GPIO_NUM_18;     //<! lcd data1 pin (QSPI D1)
static constexpr gpio_num_t kLcdData2Pin = GPIO_NUM_21;     //<! lcd data2 pin (QSPI D2)
static constexpr gpio_num_t kLcdData3Pin = GPIO_NUM_14;     //<! lcd data3 pin (QSPI D3)
static constexpr gpio_num_t kLcdSckPin = GPIO_NUM_17;       //<! lcd clock pin (QSPI SCK)
static constexpr uint8_t kTouchI2cAddress = 0x3B;           //<! touch i2c address
static constexpr gpio_num_t kTouchIntPin = GPIO_NUM_11;     //<! touch irq pin
static constexpr gpio_num_t kTouchResetPin = GPIO_NUM_16;   //<! touch reset pin

//  Use of buffers at least 1/10 display size is recommended.
static constexpr unsigned int kLcdPixelClockHz = 30 * 1000 * 1000;  //<! spi clock frequency for lcd
static constexpr uint8_t kLcdCmdBits = 32;                          //<! bit number used to represent command
static constexpr uint8_t kLcdParamBits = 8;                         //<! bit number used to represent parameter

/// @brief Framebuffer size used for LVGL, in bytes
/// @note 640 (width) x 180 (height) x 2 (bytes per pixel)
static constexpr size_t kFramebufferSize = 640 * 180 * sizeof(lv_color16_t);

/// FIXME: changing this breaks rendering... why???????
/// 640 x 20 lines = seems to be the only combination that works well
/// 16 works okish, with artifacts in the partial renders
/// 17 shows LOTS of artifacts, but a somewhat recognizable image on first render
/// 18 create LOTS of articfacts on first draw, but partial renders clean up to the same somewhat garbled image as 17
/// 19 is similar to 17
/// 20 renders a PERFECT first image, but partial renders are a bit garbled.
/// 21 is similar to 16
/// 22 is similar to 20
/// 32 is similar to 16
/// 40 draws a screen full of random noise and then has SOME partial renders show up,garbled.
static constexpr uint8_t kLvglDrawBufferLines = 18;  //<! number of display lines in each draw buffer
static constexpr size_t kMainResolutionSize = 640;   //<! main resolution size (width in pixels) used by calculations

// for portrait mode
// static constexpr uint8_t kLvglDrawBufferLines = 64;  //<! number of display lines in each draw buffer
// static constexpr size_t kMainResolutionSize = 180;   //<! main resolution size (width in pixels) used by
// calculationss

/// @brief Send buffer size used for SPI transfers, in pixels
/// @note [main resolution size] x [number of lines in draw buffer]
///
/// 14400 = vendor value
static constexpr size_t kSendBufSize = kMainResolutionSize * kLvglDrawBufferLines;

/// @brief Draw buffer size used by LVGL, in bytes
/// @note [main resolution size] x [number of lines in draw buffer] x [bytes per pixel]
static constexpr size_t kDrawBufferSize = kSendBufSize * sizeof(lv_color16_t);

// static constexpr size_t kMaxTransferSize = (kSendBufSize * 16) + 8;  //<! in bits

/// @brief Maximum transfer size for SPI transfers, in bytes
/// @note [draw buffer size] + 8 bytes overhead
static constexpr size_t kMaxTransferSize = kDrawBufferSize + 8;
// static constexpr size_t kMaxTransferSize = (kFramebufferSize) + 8;

esp_err_t DisplayPanelSetup();
esp_err_t PanelInit();
esp_err_t PanelReset();
esp_err_t LvgLBufferSetupPartial();
esp_err_t LvgLBufferSetupFull();

esp_err_t TouchPanelSetup();
void GetDisplayDimensions(uint16_t& width, uint16_t& height);
lv_display_t* GetDisplayObjPtr();
esp_lcd_panel_io_handle_t GetPanelIOHandle();

esp_err_t SetupQSPI();

void LvglFlushCallback(lv_display_t* disp, const lv_area_t* area, uint8_t* px_map);

bool LvglFlushReadyCallback(esp_lcd_panel_io_handle_t io, esp_lcd_panel_io_event_data_t* edata, void* user_data);

void LvglTouchCallback(lv_indev_t* indev, lv_indev_data_t* data);

void TurnOn();

void ShowBootScreen();

void Backlight();

void BacklightTimerCallback(void* arg);

}  // namespace impl
}  // namespace display
