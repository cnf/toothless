// cSpell: words qspi lvgl caset raset
//
// AXS15231B QSPI display driver for LilyGo T-Display S3 Long (180×640).
//
// ## Hardware constraints & workarounds
//
// 1. **QSPI-only interface** — The Espressif esp_lcd SPI panel driver does not
//    support QSPI (quad-data write with single-line cmd/addr), so we drive SPI2
//    directly using spi_device_polling_transmit with SPI_TRANS_MODE_QIO.
//
// 2. **MADCTL ignored** — The AXS15231B ignores the MADCTL (0x36) register in
//    QSPI mode, so hardware rotation is not available. We create the LVGL
//    display as 640×180 (landscape) and software-transpose to 180×640 (native
//    portrait) in the flush callback.
//
// 3. **Full-refresh required** — Partial RASET windows that don't span the full
//    column range (0–639) cause garbled output. LilyGo's own reference code
//    uses full_refresh=1 for this reason. We use LV_DISPLAY_RENDER_MODE_FULL.
//
// 4. **PSRAM→SPI DMA** — The ESP32-S3 SPI DMA cannot read directly from PSRAM.
//    A small internal-RAM bounce buffer (_transpose_buf) is used to copy chunks
//    from the PSRAM framebuffer to SPI.
//
// 5. **Byte-swap** — The panel expects big-endian RGB565; LVGL stores
//    little-endian. Bytes are swapped inline during the transpose step.
//
#include <driver/gpio.h>
#include <driver/spi_master.h>
#include <esp_check.h>
#include <esp_heap_caps.h>
#include <esp_timer.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "display_impl.hpp"
#include "funlog.h"
#include "sdkconfig.h"

static const char* TAG = FLOG_SHORT_FILENAME;

namespace impl {
namespace display {

// ---------------------------------------------------------------------------
// Internal state
// ---------------------------------------------------------------------------
static spi_device_handle_t _spi = nullptr;
static lv_display_t* _display = nullptr;

/// DMA bounce buffer for PSRAM→SPI transfers during flush.
static uint16_t* _transpose_buf = nullptr;

// ---------------------------------------------------------------------------
// Low-level QSPI helpers — matches LilyGo AXS15231B.cpp exactly
// ---------------------------------------------------------------------------

static inline void CsLow() { gpio_set_level(kLcdCsPin, 0); }
static inline void CsHigh() { gpio_set_level(kLcdCsPin, 1); }

/// Send an LCD command + optional parameter bytes (all single-line, no QIO)
static void LcdSendCmd(uint8_t cmd, const uint8_t* data, size_t len) {
  CsLow();
  spi_transaction_t t{};
  t.cmd = 0x02;
  t.addr = static_cast<uint32_t>(cmd) << 8;
  if (len > 0) {
    t.tx_buffer = data;
    t.length = len * 8;
  }
  spi_device_polling_transmit(_spi, &t);
  CsHigh();
}

/// Set the address window (CASET + RASET)
static void LcdSetWindow(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2) {
  uint8_t caset[] = {(uint8_t)(x1 >> 8), (uint8_t)x1, (uint8_t)(x2 >> 8), (uint8_t)x2};
  uint8_t raset[] = {(uint8_t)(y1 >> 8), (uint8_t)y1, (uint8_t)(y2 >> 8), (uint8_t)y2};
  LcdSendCmd(0x2A, caset, 4);
  LcdSendCmd(0x2B, raset, 4);
}

/// Push pixel data using QSPI (quad data phase).
/// Sends RAMWR (0x2C) on the first chunk, then continuation chunks
/// with no command/address (just raw quad data).
static void LcdPushPixels(const uint16_t* data, size_t pixel_count) {
  const uint16_t* p = data;
  bool first = true;

  CsLow();
  while (pixel_count > 0) {
    size_t chunk = (pixel_count > kSendBufSize) ? kSendBufSize : pixel_count;

    spi_transaction_ext_t t{};
    if (first) {
      t.base.flags = SPI_TRANS_MODE_QIO;
      t.base.cmd = 0x32;
      t.base.addr = 0x002C00;
      first = false;
    } else {
      t.base.flags = SPI_TRANS_MODE_QIO | SPI_TRANS_VARIABLE_CMD | SPI_TRANS_VARIABLE_ADDR | SPI_TRANS_VARIABLE_DUMMY;
      t.command_bits = 0;
      t.address_bits = 0;
      t.dummy_bits = 0;
    }
    t.base.tx_buffer = p;
    t.base.length = chunk * 16;  // bits (16 bits per pixel)
    spi_device_polling_transmit(_spi, reinterpret_cast<spi_transaction_t*>(&t));

    p += chunk;
    pixel_count -= chunk;
  }
  CsHigh();
}

// ---------------------------------------------------------------------------
// Init sequence (matches LilyGo axs15231b_qspi_init)
// ---------------------------------------------------------------------------

struct LcdInitCmd {
  uint8_t cmd;
  uint8_t flags;  ///< bit7 = delay 200ms, bit6 = delay 20ms
};

static constexpr LcdInitCmd kInitSeq[] = {
    {0x28, 0x40},  // DISPOFF, delay 20ms
    {0x10, 0x80},  // SLPIN,   delay 200ms
    {0x11, 0x80},  // SLPOUT,  delay 200ms
    {0x29, 0x00},  // DISPON
};

static void LcdRunInitSequence() {
  for (const auto& c : kInitSeq) {
    LcdSendCmd(c.cmd, nullptr, 0);
    if (c.flags & 0x80) vTaskDelay(pdMS_TO_TICKS(200));
    if (c.flags & 0x40) vTaskDelay(pdMS_TO_TICKS(20));
  }
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

esp_err_t SetupQSPI() {
  LV_LOG_USER("Init QSPI bus");

  gpio_set_direction(kLcdCsPin, GPIO_MODE_OUTPUT);
  gpio_set_direction(kLcdResetPin, GPIO_MODE_OUTPUT);
  CsHigh();

  const spi_bus_config_t buscfg = {
      .data0_io_num = kLcdData0Pin,
      .data1_io_num = kLcdData1Pin,
      .sclk_io_num = kLcdSckPin,
      .data2_io_num = kLcdData2Pin,
      .data3_io_num = kLcdData3Pin,
      .max_transfer_sz = (kSendBufSize * 2) + 8,
      .flags = SPICOMMON_BUSFLAG_MASTER | SPICOMMON_BUSFLAG_GPIO_PINS,
  };
  ESP_RETURN_ON_ERROR(spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_CH_AUTO), TAG, "SPI bus init failed");

  const spi_device_interface_config_t devcfg = {
      .command_bits = 8,
      .address_bits = 24,
      .mode = 0,  ///< SPI_MODE0 — LilyGo's proven setting
      .clock_speed_hz = kLcdPixelClockHz,
      .spics_io_num = -1,  ///< CS managed manually via GPIO
      .flags = SPI_DEVICE_HALFDUPLEX,
      .queue_size = 17,
  };
  ESP_RETURN_ON_ERROR(spi_bus_add_device(SPI2_HOST, &devcfg, &_spi), TAG, "SPI add device failed");

  return ESP_OK;
}

esp_err_t PanelInit() {
  LV_LOG_USER("Hardware reset");
  gpio_set_level(kLcdResetPin, 1);
  vTaskDelay(pdMS_TO_TICKS(130));
  gpio_set_level(kLcdResetPin, 0);
  vTaskDelay(pdMS_TO_TICKS(130));
  gpio_set_level(kLcdResetPin, 1);
  vTaskDelay(pdMS_TO_TICKS(300));

  LV_LOG_USER("LCD init sequence");
  LcdRunInitSequence();

  return ESP_OK;
}

esp_err_t PanelReset() {
  gpio_set_level(kLcdResetPin, 0);
  vTaskDelay(pdMS_TO_TICKS(130));
  gpio_set_level(kLcdResetPin, 1);
  vTaskDelay(pdMS_TO_TICKS(300));
  return ESP_OK;
}

/// Init LVGL with a full-screen framebuffer in PSRAM.
/// Both public entry points (Partial/Full) forward here — partial mode
/// is not viable on this panel (see file header).
static esp_err_t LvglSetup() {
  LV_LOG_USER("Init LVGL (full-refresh, 640x180 landscape)");
  lv_init();

  _display = lv_display_create(kVRes, kHRes);
  if (!_display) {
    LV_LOG_ERROR("Failed to create LVGL display");
    return ESP_ERR_NO_MEM;
  }
  lv_display_set_dpi(_display, kLcdDPI);

  /// DMA bounce buffer — internal RAM for PSRAM→SPI transfer
  _transpose_buf = static_cast<uint16_t*>(heap_caps_malloc(kDrawBufferSize, MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL));
  assert(_transpose_buf);

  /// Full-screen LVGL framebuffer in PSRAM
  void* buf1 = heap_caps_malloc(kFramebufferSize, MALLOC_CAP_SPIRAM);
  assert(buf1);

  lv_display_set_buffers(_display, buf1, nullptr, kFramebufferSize, LV_DISPLAY_RENDER_MODE_FULL);
  lv_display_set_flush_cb(_display, LvglFlushCallback);

  return ESP_OK;
}

esp_err_t LvgLBufferSetupPartial() { return LvglSetup(); }
esp_err_t LvgLBufferSetupFull() { return LvglSetup(); }

esp_err_t DisplayPanelSetup() {
  ESP_RETURN_ON_ERROR(SetupQSPI(), TAG, "QSPI bus init failed");
  ESP_RETURN_ON_ERROR(PanelInit(), TAG, "Panel init failed");
  ESP_RETURN_ON_ERROR(LvglSetup(), TAG, "LVGL setup failed");
  TurnOn();
  return ESP_OK;
}

void LvglFlushCallback(lv_display_t* disp, const lv_area_t* /*area*/, uint8_t* px_map) {
  /// Full-refresh mode: LVGL gives us the entire 640×180 landscape framebuffer.
  /// Transpose 90° CW to 180×640 portrait, byte-swap RGB565, bounce via DMA to SPI.
  const auto* src = reinterpret_cast<const uint16_t*>(px_map);

  LcdSetWindow(0, 0, kHRes - 1, kVRes - 1);

  /// Panel receives pixels in row-major order: 640 rows × 180 cols.
  /// Panel pixel (row, col) ← landscape pixel (lx=639-row, ly=col) ← src[ly * kVRes + lx].
  static constexpr uint32_t kTotalPixels = kHRes * kVRes;  ///< 180 × 640 = 115200
  static constexpr uint32_t kBounceCap = kSendBufSize;     ///< 12800 pixels
  uint32_t pixels_done = 0;
  bool first = true;

  CsLow();
  while (pixels_done < kTotalPixels) {
    uint32_t chunk = kTotalPixels - pixels_done;
    if (chunk > kBounceCap) chunk = kBounceCap;

    /// Transpose chunk: convert linear panel index → landscape source pixel
    for (uint32_t i = 0; i < chunk; ++i) {
      const uint32_t p = pixels_done + i;
      const uint32_t prow = p / kHRes;            ///< panel row [0..639]
      const uint32_t pcol = p % kHRes;            ///< panel col [0..179]
      const uint32_t lx = (kVRes - 1) - prow;     ///< landscape x
      const uint32_t ly = pcol;                   ///< landscape y
      const uint16_t px = src[ly * kVRes + lx];   ///< stride = kVRes (640)
      _transpose_buf[i] = (px << 8) | (px >> 8);  ///< byte-swap for panel
    }

    spi_transaction_ext_t t{};
    if (first) {
      t.base.flags = SPI_TRANS_MODE_QIO;
      t.base.cmd = 0x32;
      t.base.addr = 0x002C00;
      first = false;
    } else {
      t.base.flags = SPI_TRANS_MODE_QIO | SPI_TRANS_VARIABLE_CMD | SPI_TRANS_VARIABLE_ADDR | SPI_TRANS_VARIABLE_DUMMY;
      t.command_bits = 0;
      t.address_bits = 0;
      t.dummy_bits = 0;
    }
    t.base.tx_buffer = _transpose_buf;
    t.base.length = chunk * 16;
    spi_device_polling_transmit(_spi, reinterpret_cast<spi_transaction_t*>(&t));

    pixels_done += chunk;
  }
  CsHigh();

  lv_display_flush_ready(disp);
}

/// Unused — required by shared header. Flush is synchronous on this panel.
bool LvglFlushReadyCallback(esp_lcd_panel_io_handle_t, esp_lcd_panel_io_event_data_t*, void*) { return false; }

void GetDisplayDimensions(uint16_t& width, uint16_t& height) {
  width = kHRes;
  height = kVRes;
}

lv_display_t* GetDisplayObjPtr() { return _display; }

esp_lcd_panel_io_handle_t GetPanelIOHandle() { return nullptr; }  ///< No esp_lcd panel IO used

void TurnOn() {
  lv_async_call(
      [](void*) {
        ShowBootScreen();
        BacklightSetup();
      },
      nullptr);
}

void ShowBootScreen() {
  LV_LOG_USER("Displaying boot screen");
  lv_obj_t* scr = lv_obj_create(nullptr);
  lv_obj_set_style_bg_color(scr, lv_color_black(), 0);
  lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);

  lv_obj_t* label = lv_label_create(scr);
  lv_label_set_text(label, "Toothless");
  lv_obj_set_style_text_font(label, &lv_font_montserrat_32, 0);
  lv_obj_center(label);

  lv_obj_t* sub = lv_label_create(scr);
  lv_label_set_text(sub, "Initializing...");
  lv_obj_set_style_text_font(sub, &lv_font_montserrat_20, 0);
  lv_obj_align_to(sub, label, LV_ALIGN_OUT_BOTTOM_MID, 0, 10);

  lv_screen_load(scr);
}

/// Debug helper — draws colored bands directly to panel, bypassing LVGL.
void TestPanelGeometry() {
  constexpr size_t kBandH = 32;
  auto* buf = static_cast<uint16_t*>(heap_caps_malloc(kHRes * kBandH * 2, MALLOC_CAP_DMA));
  if (!buf) return;

  const uint16_t colors[] = {0xF800, 0x07E0, 0x001F, 0xFFFF};  // R G B W
  for (size_t band = 0; band < 4 && band * kBandH < (size_t)kVRes; ++band) {
    for (size_t i = 0; i < kHRes * kBandH; ++i) buf[i] = colors[band];
    uint16_t y0 = band * kBandH;
    LcdSetWindow(0, y0, kHRes - 1, y0 + kBandH - 1);
    LcdPushPixels(buf, kHRes * kBandH);
    vTaskDelay(pdMS_TO_TICKS(50));
  }
  heap_caps_free(buf);
}

}  // namespace display
}  // namespace impl