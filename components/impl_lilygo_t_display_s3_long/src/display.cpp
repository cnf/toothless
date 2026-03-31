// cSpell: words qspi lvgl
//
// Raw SPI implementation matching LilyGo's proven QSPI protocol:
//   - SPI_MODE0, command_bits=8, address_bits=24, half-duplex
//   - Manual CS via GPIO
//   - QIO flag only on pixel data phase
//   - Minimal init: HW reset → DISPOFF → SLPIN → SLPOUT → DISPON
//
#include <driver/gpio.h>
#include <driver/spi_master.h>
#include <esp_check.h>
#include <esp_heap_caps.h>
#include <esp_timer.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include <cstring>

#include "display_impl.hpp"
#include "funlog.h"
#include "sdkconfig.h"

static const char* TAG = "display_long";

namespace impl {
namespace display {

// ---------------------------------------------------------------------------
// Internal state
// ---------------------------------------------------------------------------
static spi_device_handle_t _spi = nullptr;
static lv_display_t* _display = nullptr;
static esp_lcd_panel_io_handle_t _io_handle = nullptr;  ///< unused, kept for header compat

/// Max pixels per DMA chunk (matches LilyGo SEND_BUF_SIZE = 14400 pixels)
static constexpr size_t kSpiChunkPixels = 14400;

/// Transpose buffer — allocated once, used by flush to rotate landscape→portrait
/// Max flush area: full display width (640) × kLvglDrawBufferLines (20) = 12800 pixels
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
    size_t chunk = (pixel_count > kSpiChunkPixels) ? kSpiChunkPixels : pixel_count;

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
      .max_transfer_sz = (kSpiChunkPixels * 2) + 8,
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

esp_err_t LvgLBufferSetupPartial() {
  LV_LOG_USER("Init LVGL");
  lv_init();

  /// LVGL sees 640×180 landscape; flush transposes to 180×640 portrait panel
  _display = lv_display_create(kVRes, kHRes);
  if (!_display) {
    LV_LOG_ERROR("Failed to create LVGL display");
    return ESP_ERR_NO_MEM;
  }
  lv_display_set_dpi(_display, kLcdDPI);
  LV_LOG_USER("Display: %lix%li @ %u DPI (landscape, flush rotates)", kVRes, kHRes, (unsigned)kLcdDPI);

  /// Transpose buffer: same size as draw buffer, must be DMA-capable for SPI
  _transpose_buf = static_cast<uint16_t*>(heap_caps_malloc(kDrawBufferSize, MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL));
  assert(_transpose_buf);
  LV_LOG_USER("Transpose buffer: %u bytes @ %p (DMA)", (unsigned)kDrawBufferSize, _transpose_buf);

  void* buf1 = heap_caps_malloc(kDrawBufferSize, MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);
  if (!buf1) {
    LV_LOG_WARN("DMA alloc failed for buf1, trying PSRAM");
    buf1 = heap_caps_malloc(kDrawBufferSize, MALLOC_CAP_SPIRAM);
  }
  void* buf2 = heap_caps_malloc(kDrawBufferSize, MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);
  if (!buf2) {
    LV_LOG_WARN("DMA alloc failed for buf2, trying PSRAM");
    buf2 = heap_caps_malloc(kDrawBufferSize, MALLOC_CAP_SPIRAM);
  }
  assert(buf1 && buf2);
  LV_LOG_USER("Buffers: %u bytes each (buf1=%p buf2=%p)", (unsigned)kDrawBufferSize, buf1, buf2);

  lv_display_set_buffers(_display, buf1, buf2, kDrawBufferSize, LV_DISPLAY_RENDER_MODE_PARTIAL);
  lv_display_set_flush_cb(_display, LvglFlushCallback);

  return ESP_OK;
}

esp_err_t LvgLBufferSetupFull() { return LvgLBufferSetupPartial(); }

esp_err_t DisplayPanelSetup() {
  ESP_ERROR_CHECK(gpio_set_direction(kLcdBacklightPin, GPIO_MODE_OUTPUT));
  gpio_set_level(kLcdBacklightPin, 1);  // leave this here for now, for debugging, so i can see what is going on
  ESP_RETURN_ON_ERROR(SetupQSPI(), TAG, "QSPI bus init failed");
  ESP_RETURN_ON_ERROR(PanelInit(), TAG, "Panel init failed");
  ESP_RETURN_ON_ERROR(LvgLBufferSetupPartial(), TAG, "LVGL buffer setup failed");
  return ESP_OK;
}

esp_err_t TouchPanelSetup() {
  LV_LOG_USER("Touch: not yet implemented");
  return ESP_OK;
}

void LvglFlushCallback(lv_display_t* disp, const lv_area_t* area, uint8_t* px_map) {
  /// LVGL works in landscape: x=[0..639], y=[0..179]
  /// Panel is portrait:       col=[0..179], row=[0..639]
  /// Mapping (90° CW): panel_col = ly, panel_row = (kVRes-1) - lx
  const uint16_t lx1 = area->x1;
  const uint16_t ly1 = area->y1;
  const uint16_t lx2 = area->x2;
  const uint16_t ly2 = area->y2;
  const uint16_t lw = lx2 - lx1 + 1;  ///< landscape width of this area
  const uint16_t lh = ly2 - ly1 + 1;  ///< landscape height of this area

  const auto* src = reinterpret_cast<const uint16_t*>(px_map);

  /// Transposed panel window:
  ///   panel_col = ly1 .. ly2   (landscape Y → panel X)
  ///   panel_row = (kVRes-1-lx2) .. (kVRes-1-lx1)  (landscape X → panel Y, flipped)
  const uint16_t pcol1 = ly1;
  const uint16_t pcol2 = ly2;
  const uint16_t prow1 = (kVRes - 1) - lx2;
  const uint16_t prow2 = (kVRes - 1) - lx1;
  const uint16_t pw = pcol2 - pcol1 + 1;  ///< = lh
  const uint16_t ph = prow2 - prow1 + 1;  ///< = lw

  /// Transpose pixel data: src is lw×lh row-major (landscape)
  /// dst needs to be pw×ph row-major (portrait panel)
  /// dst[pr][pc] = src[(kVRes-1-lx1) - (prow1+pr)][pc - pcol1 + ly1]
  /// Simplified: dst row pr corresponds to landscape x = lx2 - pr (top-to-bottom flip)
  for (uint16_t pr = 0; pr < ph; ++pr) {
    const uint16_t lx = lx2 - pr;  ///< landscape x, scanning right-to-left
    for (uint16_t pc = 0; pc < pw; ++pc) {
      const uint16_t ly = ly1 + pc;  ///< landscape y
      _transpose_buf[pr * pw + pc] = src[(ly - ly1) * lw + (lx - lx1)];
    }
  }

  /// Fix byte order: ESP32 little-endian → panel big-endian RGB565
  lv_draw_sw_rgb565_swap(_transpose_buf, (uint32_t)pw * ph);

  ESP_LOGI(TAG, "flush: lvgl(%u,%u)-(%u,%u) %ux%u → panel col(%u-%u) row(%u-%u) %ux%u px=%u", lx1, ly1, lx2, ly2, lw,
           lh, pcol1, pcol2, prow1, prow2, pw, ph, (unsigned)(pw * ph));

  LcdSetWindow(pcol1, prow1, pcol2, prow2);
  LcdPushPixels(_transpose_buf, (uint32_t)pw * ph);

  lv_display_flush_ready(disp);
}

bool LvglFlushReadyCallback(esp_lcd_panel_io_handle_t io, esp_lcd_panel_io_event_data_t* edata, void* user_data) {
  lv_display_t* disp = static_cast<lv_display_t*>(user_data);
  lv_display_flush_ready(disp);
  return false;
}

void LvglTouchCallback(lv_indev_t* /*indev*/, lv_indev_data_t* data) { data->state = LV_INDEV_STATE_RELEASED; }

void GetDisplayDimensions(uint16_t& width, uint16_t& height) {
  width = kHRes;
  height = kVRes;
}

lv_display_t* GetDisplayObjPtr() { return _display; }

esp_lcd_panel_io_handle_t GetPanelIOHandle() { return _io_handle; }

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

void Backlight() {
  esp_timer_handle_t timer = nullptr;
  const esp_timer_create_args_t args = {.callback = BacklightTimerCallback, .arg = nullptr, .name = "bl_timer"};
  ESP_ERROR_CHECK(esp_timer_create(&args, &timer));
  ESP_ERROR_CHECK(esp_timer_start_once(timer, 100000));
}

void BacklightTimerCallback(void* arg) {
  gpio_set_direction(kLcdBacklightPin, GPIO_MODE_OUTPUT);
  gpio_set_level(kLcdBacklightPin, 1);
}

void TestPanelGeometry() {
  /// Draw colored bands to verify panel geometry — bypasses LVGL
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