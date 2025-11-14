#pragma once

#include <driver/spi_master.h>
#include <esp_lcd_panel_io.h>
#include <hal/spi_types.h>
#include <lvgl.h>

#include <memory>
// #include <mutex>

// #define DISPLAY_BL_ON_LEVEL 1

// #define BOARD_DISP_CS (12)
// #define BOARD_DISP_SCK (17)
// #define BOARD_DISP_DATA0 (13)
// #define BOARD_DISP_DATA1 (18)
// #define BOARD_DISP_DATA2 (21)
// #define BOARD_DISP_DATA3 (14)
// #define BOARD_DISP_RESET (16)
// #define BOARD_DISP_TE (-1)
// #define BOARD_DISP_BL (1)

// #define BOARD_I2C_SDA (15)
// #define BOARD_I2C_SCL (10)

// #define BOARD_TOUCH_IRQ (11)
// #define BOARD_TOUCH_RST (16)

// #define AMOLED_HEIGHT (180)
// #define AMOLED_WIDTH (640)

// #define BOARD_HAS_TOUCH 1

// #define DISPLAY_FULLRESH true
// #define DISPLAY_BUFFER_SIZE (AMOLED_WIDTH * AMOLED_HEIGHT)

// #define DEFAULT_SCK_SPEED (30000000)
// #define SEND_BUF_SIZE         (28800/2) //16bit(RGB565)

namespace display {
namespace impl {

static constexpr int32_t kHres = CONFIG_IMPL_LILYGO_TDISPLAY_S3_LONG_HRES;
static constexpr int32_t kVres = CONFIG_IMPL_LILYGO_TDISPLAY_S3_LONG_VRES;

static constexpr gpio_num_t kLcdBacklightPin = GPIO_NUM_1;  //<! lcd backlight pin
static constexpr gpio_num_t kLcdCsPin = GPIO_NUM_12;        //<! lcd chip select pin
static constexpr gpio_num_t kLcdResetPin = GPIO_NUM_16;     //<! lcd reset pin
static constexpr gpio_num_t kLcdTePin = GPIO_NUM_NC;        //<! lcd te pin
static constexpr gpio_num_t kLcdData0Pin = GPIO_NUM_13;     //<! lcd data0 pin (QSPI D0)
static constexpr gpio_num_t kLcdData1Pin = GPIO_NUM_18;     //<! lcd data1 pin (QSPI D1)
static constexpr gpio_num_t kLcdData2Pin = GPIO_NUM_21;     //<! lcd data2 pin (QSPI D2)
static constexpr gpio_num_t kLcdData3Pin = GPIO_NUM_14;     //<! lcd data3 pin (QSPI D3)
static constexpr gpio_num_t kLcdSckPin = GPIO_NUM_17;       //<! lcd clock pin (QSPI SCK)
static constexpr gpio_num_t kTouchIntPin = GPIO_NUM_11;     //<! touch irq pin
static constexpr gpio_num_t kTouchResetPin = GPIO_NUM_16;   //<! touch reset pin

// static constexpr uint32_t kDefaultClockSpeed = 30 * 1000 * 1000;  //<! default spi clock frequency

static constexpr unsigned int kLcdPixelClockHz = 20 * 1000 * 1000;  //<! spi clock frequency for lcd < default is 40?
static constexpr uint8_t kLcdCmdBits = 32;                          //<! bit number used to represent command
static constexpr uint8_t kLcdParamBits = 8;                         //<! bit number used to represent parameter
static constexpr uint8_t kLvglDrawBufferLines = 10;                 // number of display lines in each draw buffer
static constexpr size_t kDrawBufferSize = CONFIG_IMPL_LILYGO_TDISPLAY_S3_LONG_HRES * kLvglDrawBufferLines *
                                          sizeof(uint16_t);           // FIXME: this is probably wrong for this display
static constexpr size_t kSendBufSize = kHres * kLvglDrawBufferLines;  //<! in pixels! [ 14400 = vendor value]
static constexpr size_t kMaxTransferSize = (kSendBufSize * 16) + 8;   //<! in bits

esp_err_t DisplayPanelSetup();
esp_err_t TouchPanelSetup();
void GetDisplayDimensions(uint16_t& width, uint16_t& height);
lv_display_t* GetDisplayObjPtr();

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
