// cSpell: words lvgl qspi
#include <driver/gpio.h>
#include <driver/spi_master.h>
#include <esp_check.h>
#include <esp_lcd_axs15231b.h>
// #include <esp_lcd_panel_ops.h>
// #include <esp_lcd_panel_vendor.h>
#include <esp_cache.h>
#include <esp_task_wdt.h>
#include <esp_timer.h>
#include <lvgl.h>

#include "display_commands.hpp"
#include "display_impl.hpp"
#include "funlog.h"
#include "i2c_manager.hpp"
#include "sdkconfig.h"

///////////////////////////////////////////////

#include <inttypes.h>
#include <string.h>
#include <sys/param.h>

#include "esp_cache.h"
#include "esp_check.h"
#include "esp_compiler.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_private/critical_section.h"
#include "esp_private/esp_cache_private.h"
#include "esp_rom_caps.h"
#include "freertos/FreeRTOS.h"
#include "hal/cache_hal.h"
#include "hal/cache_ll.h"
#include "hal/mmu_hal.h"
#include "sdkconfig.h"
#include "soc/soc_caps.h"
#include "sys/lock.h"

namespace display {
namespace impl {

#define delay(ms) vTaskDelay(ms / portTICK_PERIOD_MS)

static lv_display_t* _display;
static spi_device_handle_t _spi = NULL;

static void* _rotationbuffer = nullptr;  // Persistent 640×180 framebuffer in PSRAM

static uint8_t* _rotated_buf = nullptr;
static uint8_t* _contiguous_buf = nullptr;

i2c_master_dev_handle_t _dev_handle;
i2c_master_bus_handle_t _bus_handle;

typedef struct {
  uint32_t addr;
  uint8_t param[20];
  uint32_t len;
} lcd_cmd_t;

// if (lcd_init[i].len & 0x80) delay(200);
// if (lcd_init[i].len & 0x40) delay(20);
static const lcd_cmd_t vendor_config[] = {
    {0x28, {0x00}, 0x40},  //<! DISP OFF - Delay 20
    {0x10, {0x00}, 0x20},  //<! SLPIN
    {0x11, {0x00}, 0x80},  //<! SLPOUT - Delay 200
    {0x29, {0x00}, 0x00},  //<! DISP ON
};

static void inline setCS() { gpio_set_level(kLcdCsPin, 0); }

static void inline clrCS() { gpio_set_level(kLcdCsPin, 1); }

// static void WriteComm(uint8_t data) {
//   setCS();
//   SPI.beginTransaction(SPISettings(SPI_FREQUENCY, MSBFIRST, TFT_SPI_MODE));
//   SPI.write(0x00);
//   SPI.write(data);
//   SPI.write(0x00);
//   SPI.endTransaction();
//   clrCS();
// }

// void write_comm(spi_device_handle_t spi, uint8_t data) {
//   uint8_t buf[3] = {0x00, data, 0x00};

//   spi_transaction_t t = {
//       .length = 3 * 8,
//       .tx_buffer = buf,
//   };

//   // If CS must be manually controlled, tell me; otherwise SPI driver handles it.
//   spi_device_transmit(spi, &t);
// }

static void amoled_write_cmd(uint32_t cmd, const uint8_t* pdat, uint32_t length) {
  setCS();
  spi_transaction_t t;
  memset(&t, 0, sizeof(t));
  t.flags = (SPI_TRANS_MULTILINE_CMD | SPI_TRANS_MULTILINE_ADDR);
  if (cmd == 0xff && length == 0x1f) {
    t.cmd = 0x02;
    t.addr = 0xffff;
    length = 0;
  } else if (cmd == 0x00) {
    t.cmd = 0x00;
    t.addr = 0x0000;
    length = 4;
  } else {
    t.cmd = 0x02;
    t.addr = cmd << 8;
  }
  // t.cmd = 0x02;
  // t.addr = cmd << 8;
  if (length != 0) {
    t.tx_buffer = pdat;
    t.length = 8 * length;
  } else {
    t.tx_buffer = NULL;
    t.length = 0;
  }
  spi_device_polling_transmit(_spi, &t);
  clrCS();
}

// static void amoled_push_buffer(uint16_t* data, uint32_t len) {
//   bool first_send = true;
//   uint16_t* p = data;
//   assert(p);
//   assert(_spi);
//   do {
//     setCS();
//     size_t chunk_size = len;
//     spi_transaction_ext_t t = {0};

//     memset(&t, 0, sizeof(t));
//     t.base.flags = SPI_TRANS_MODE_QIO;
//     t.base.cmd = 0x32;

//     if (first_send) {
//       t.base.addr = 0x002C00;
//       first_send = false;
//     } else {
//       t.base.addr = 0x003C00;
//     }

//     if (chunk_size > kSendBufSize) {
//       chunk_size = kSendBufSize;
//     }

//     t.base.tx_buffer = p;
//     t.base.length = chunk_size * 16;

//     spi_device_polling_transmit(_spi, (spi_transaction_t*)&t);
//     clrCS();

//     len -= chunk_size;
//     p += chunk_size;
//   } while (len > 0);
// }
// static void amoled_push_buffer(uint16_t* data, uint32_t len) {
//   bool first_send = true;
//   uint16_t* p = data;
//   assert(p);
//   assert(_spi);

//   do {
//     setCS();
//     size_t chunk_size = len;
//     spi_transaction_ext_t t = {0};

//     memset(&t, 0, sizeof(t));
//     t.base.flags = SPI_TRANS_MODE_QIO;
//     t.base.cmd = 0x32;

//     if (first_send) {
//       t.base.addr = 0x002C00;
//       first_send = false;
//     } else {
//       t.base.addr = 0x003C00;
//     }

//     if (chunk_size > kSendBufSize) {
//       chunk_size = kSendBufSize;
//     }

//     t.base.tx_buffer = p;
//     t.base.length = chunk_size * 16;

//     spi_device_polling_transmit(_spi, (spi_transaction_t*)&t);
//     clrCS();  // ← CS toggle happens AFTER transmission, not before

//     len -= chunk_size;
//     p += chunk_size;
//   } while (len > 0);
// }

// static void amoled_push_buffer(uint16_t* data, uint32_t len) {
//   bool first_send = true;
//   uint16_t* p = data;

//   // Cache sync
//   uintptr_t addr = (uintptr_t)data;
//   uintptr_t aligned_addr = addr & ~(32 - 1);
//   size_t size = len * sizeof(uint16_t);
//   size_t aligned_size = (size + (addr - aligned_addr) + 31) & ~(32 - 1);
//   esp_cache_msync((void*)aligned_addr, aligned_size, ESP_CACHE_MSYNC_FLAG_DIR_C2M);

//   setCS();

//   do {
//     spi_transaction_ext_t t = {0};
//     t.base.flags = SPI_TRANS_MODE_QIO;
//     t.base.cmd = 0x32;

//     if (first_send) {
//       t.base.addr = 0x002C00;
//       first_send = false;
//     } else {
//       t.base.addr = 0x003C00;
//     }

//     size_t chunk_size = (len > kSendBufSize) ? kSendBufSize : len;

//     // // Skip first 4 pixels of THIS chunk
//     // uint16_t* chunk_start = p + 4;      // Skip 4 pixels (8 bytes)
//     // size_t send_size = chunk_size - 4;  // Send 4 fewer pixels

//     // t.base.tx_buffer = chunk_start;  // Start 4 pixels into the chunk
//     // t.base.length = send_size * 16;  // Adjust bit count

//     spi_device_polling_transmit(_spi, (spi_transaction_t*)&t);

//     len -= chunk_size;  // Still advance full chunk size
//     p += chunk_size;
//   } while (len > 0);

//   clrCS();
// }

// static void amoled_push_buffer_hmmm(uint16_t* data, uint32_t len) {
//   bool first_send = true;
//   uint16_t* p = data;
//   int chunk_num = 0;  //<! for debug logging only

//   // Calculate aligned address and size for cache sync
//   uintptr_t addr = (uintptr_t)data;
//   uintptr_t aligned_addr = addr & ~(32 - 1);  // Align down to 32-byte boundary
//   size_t size = len * sizeof(uint16_t);
//   size_t aligned_size = (size + (addr - aligned_addr) + 31) & ~(32 - 1);  // Round up to 32-byte boundary

//   // Flush CPU cache to ensure PSRAM has latest pixel data
//   esp_cache_msync((void*)aligned_addr, aligned_size, ESP_CACHE_MSYNC_FLAG_DIR_C2M);

//   setCS();

//   // // color invert byte swap
//   // for (uint32_t i = 0; i < len; i++) {
//   //   data[i] = (data[i] << 8) | (data[i] >> 8);  // Swap bytes
//   // }

//   do {
//     spi_transaction_ext_t t = {0};
//     t.base.flags = SPI_TRANS_MODE_QIO;
//     t.base.cmd = 0x32;

//     if (first_send) {
//       t.base.addr = 0x002C00;
//       first_send = false;
//     } else {
//       t.base.addr = 0x003C00;
//     }
//     LV_LOG_INFO("address: 0x%06X", t.base.addr);

//     size_t chunk_size = (len > kSendBufSize) ? kSendBufSize : len;
//     t.base.tx_buffer = p;
//     t.base.length = chunk_size * 16;

//     LV_LOG_INFO("Push chunk %d: %zu pixels (%zu bits), first_send=%d", chunk_num, chunk_size, chunk_size * 16,
//               first_send);
//     chunk_num++;

//     spi_device_polling_transmit(_spi, (spi_transaction_t*)&t);
//     // esp_rom_delay_us(1000);  // Small delay to ensure proper timing between chunks
//     vTaskDelay(1 / portTICK_PERIOD_MS);  // FIXME: has no effect

//     len -= chunk_size;
//     p += chunk_size;
//   } while (len > 0);

//   clrCS();
// }

// static void amoled_push_buffer_no_change(uint16_t* data, uint32_t len) {
//   uintptr_t addr = (uintptr_t)data;
//   uintptr_t aligned_addr = addr & ~(32 - 1);
//   size_t size = len * sizeof(uint16_t);
//   size_t aligned_size = (size + (addr - aligned_addr) + 31) & ~(32 - 1);
//   esp_cache_msync((void*)aligned_addr, aligned_size, ESP_CACHE_MSYNC_FLAG_DIR_C2M);

//   bool first_send = true;
//   uint16_t* p = data;

//   setCS();

//   do {
//     spi_transaction_ext_t t;
//     memset(&t, 0, sizeof(t));  // ← CRITICAL: Zero out the entire structure

//     t.base.flags = SPI_TRANS_MODE_QIO;
//     t.base.cmd = 0x32;

//     if (first_send) {
//       t.base.addr = 0x002C00;
//       first_send = false;
//     } else {
//       t.base.addr = 0x003C00;
//     }

//     size_t chunk_size = (len > kSendBufSize) ? kSendBufSize : len;
//     t.base.tx_buffer = p;
//     t.base.length = chunk_size * 16;

//     // Queue and immediately wait for completion
//     ESP_ERROR_CHECK(spi_device_queue_trans(_spi, (spi_transaction_t*)&t, portMAX_DELAY));

//     spi_transaction_t* ret_trans;
//     ESP_ERROR_CHECK(spi_device_get_trans_result(_spi, &ret_trans, portMAX_DELAY));

//     len -= chunk_size;
//     p += chunk_size;
//   } while (len > 0);

//   clrCS();
// }

// static void amoled_push_buffer_only_updates_same_part(uint16_t* data, uint32_t len) {
//   uint16_t* p = data;
//   int chunk_num = 0;

//   // Cache sync ONCE
//   uintptr_t addr = (uintptr_t)data;
//   uintptr_t aligned_addr = addr & ~(32 - 1);
//   size_t size = len * sizeof(uint16_t);
//   size_t aligned_size = (size + (addr - aligned_addr) + 31) & ~(32 - 1);
//   esp_cache_msync((void*)aligned_addr, aligned_size, ESP_CACHE_MSYNC_FLAG_DIR_C2M);

//   do {
//     setCS();

//     spi_transaction_ext_t t = {0};
//     t.base.flags = SPI_TRANS_MODE_QIO;
//     t.base.cmd = 0x32;
//     t.base.addr = 0x002C00;  // ALWAYS use RAMWR, never RAMWRC

//     size_t chunk_size = (len > kSendBufSize) ? kSendBufSize : len;
//     t.base.tx_buffer = p;
//     t.base.length = chunk_size * 16;

//     spi_device_polling_transmit(_spi, (spi_transaction_t*)&t);

//     clrCS();

//     LV_LOG_INFO("Chunk %d: %zu pixels sent", chunk_num++, chunk_size);

//     len -= chunk_size;
//     p += chunk_size;
//   } while (len > 0);
// }

// Data must be 32 byte aligned
static void amoled_push_buffer(uint16_t* data, uint32_t len) {
  bool first_send = true;
  uint16_t* p = data;
  int chunk_num = 0;  //<! for debug logging only

  // Calculate aligned address and size for cache sync
  uintptr_t addr = (uintptr_t)data;
  size_t sizeBytes = len * sizeof(uint16_t);

  void* alignedData = data;
  size_t alignedLen = ((len >> 6) + 1) << 6;

  uint32_t vaddr = (uint32_t)addr;
  bool valid = false;
  uint32_t cache_level = 0;
  uint32_t cache_id = 0;
  valid = cache_hal_vaddr_to_cache_level_id(vaddr, alignedLen, &cache_level, &cache_id);

  cache_type_t cache_type = CACHE_TYPE_DATA;
  uint32_t cache_line_size = cache_hal_get_cache_line_size(cache_level, cache_type);
  bool aligned_addr = (((uint32_t)addr % cache_line_size) == 0) && ((alignedLen % cache_line_size) == 0);
  if (!aligned_addr) {
    LV_LOG_INFO("Doom");
  }

  // Flush CPU cache to ensure PSRAM has latest pixel data
  esp_cache_msync(alignedData, alignedLen, ESP_CACHE_MSYNC_FLAG_DIR_C2M);

  // color invert byte swap
  for (uint32_t i = 0; i < len; i++) {
    data[i] = (data[i] << 8) | (data[i] >> 8);  // Swap bytes
  }

  while (len > 0) {
    setCS();

    spi_transaction_ext_t t = {0};
    t.base.flags = SPI_TRANS_MODE_QIO;
    t.base.cmd = 0x32;

    if (first_send) {
      t.base.addr = 0x002C00;
      first_send = false;
    } else {
      t.base.addr = 0x003C00;
    }
    LV_LOG_INFO("address: 0x%06X", t.base.addr);

    size_t chunk_size = (len > kSendBufSize) ? kSendBufSize : len;
    t.base.tx_buffer = p;
    t.base.length = chunk_size * 16;

    LV_LOG_INFO("Push chunk %d: %zu pixels (%zu bits), first_send=%d", chunk_num, chunk_size, chunk_size * 16,
                first_send);
    chunk_num++;

    spi_device_polling_transmit(_spi, (spi_transaction_t*)&t);
    // esp_rom_delay_us(1000);  // Small delay to ensure proper timing between chunks
    // vTaskDelay(1 / portTICK_PERIOD_MS);

    clrCS();

    len -= chunk_size;
    p += chunk_size;
  }
}

// static void amoled_push_buffer_chunked(uint16_t* data, uint32_t len) {
//   bool first_send = true;
//   uint16_t* p = data;
//   int chunk_num = 0;  //<! for debug logging only
//   assert(p);
//   assert(_spi);
//   do {
//     setCS();
//     size_t chunk_size = len;
//     spi_transaction_ext_t t = {0};

//     memset(&t, 0, sizeof(t));
//     t.base.flags = SPI_TRANS_MODE_QIO;
//     t.base.cmd = 0x32;

//     if (first_send) {
//       t.base.addr = 0x002C00;
//     } else {
//       t.base.addr = 0x003C00;
//     }
//     first_send = false;

//     if (chunk_size > kSendBufSize) {
//       chunk_size = kSendBufSize;
//     }

//     t.base.tx_buffer = p;
//     t.base.length = chunk_size * 16;  //<!  in BITS
//     if (!first_send) {
//       clrCS();
//     }

//     setCS();

//     spi_device_polling_transmit(_spi, (spi_transaction_t*)&t);
//     len -= chunk_size;
//     p += chunk_size;
//     LV_LOG_INFO("Push chunk %d: %zu pixels (%zu bits), first_send=%d", chunk_num++, chunk_size, chunk_size * 16,
//               first_send);
//   } while (len > 0);
//   clrCS();
// }

static void amoled_set_window(uint16_t xs, uint16_t ys, uint16_t xe, uint16_t ye) {
  lcd_cmd_t t[2] = {
      {0x2A,
       {(uint8_t)((xs >> 8) & 0xFF), (uint8_t)(xs & 0xFF), (uint8_t)((xe >> 8) & 0xFF), (uint8_t)(xe & 0xFF)},
       0x04},
      {0x2B,
       {(uint8_t)((ys >> 8) & 0xFF), (uint8_t)(ys & 0xFF), (uint8_t)((ye >> 8) & 0xFF), (uint8_t)(ye & 0xFF)},
       0x04},
  };

  for (uint32_t i = 0; i < sizeof(t) / sizeof(t[0]); i++) {
    amoled_write_cmd(t[i].addr, t[i].param, t[i].len);
  }
  // Small delay to let panel process window commands
  esp_rom_delay_us(10);  // FIXME: useless, remove
}

// void display_push_colors(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint16_t* data) {
//   amoled_set_window(x, y, x + width - 1, y + height - 1);

//   amoled_push_buffer(data, width * height);
// }

void display_push_colors(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint16_t* data) {
  // LV_LOG_USER("Window: x=%d y=%d w=%d h=%d pixels=%ld", x, y, width, height, (long)width * height);
  amoled_set_window(x, y, x + width - 1, y + height - 1);
  amoled_push_buffer(data, width * height);
}
// void display_push_colors(uint16_t x_start, uint16_t y_start, uint16_t x_end, uint16_t y_end, uint16_t* data) {
//   // LV_LOG_USER("Window: x=%d y=%d w=%d h=%d pixels=%ld", x, y, width, height, (long)width * height);
//   amoled_set_window(x_start, y_start, x_end, y_end);
//   amoled_push_buffer(data, width * height);
// }
//////////////////////////////////////////////////////

esp_err_t DisplayPanelSetup() {
  LV_LOG_INFO("Setting up display panel");
  // ESP_ERROR_CHECK(gpio_set_direction(kLcdBacklightPin, GPIO_MODE_OUTPUT));
  // gpio_set_level(kLcdBacklightPin, 1);  // BUG: Remove here

  ESP_ERROR_CHECK(PanelInit());
  lv_init();

  _display = lv_display_create(kHRes, kVRes);

  if (!_display) {
    LV_LOG_ERROR("Failed to create LVGL display");
    return ESP_ERR_INVALID_STATE;
  }
  lv_display_set_dpi(_display, kLcdDPI);
  lv_display_set_rotation(_display, LV_DISPLAY_ROTATION_90);

  LV_LOG_USER("Display resolution: %lix%li", kHRes, kVRes);

  // LvgLBufferSetupPartial();
  LvgLBufferSetupFull();

  LV_LOG_USER("Assign Flush Callback");

  // set the callback which can copy the rendered image to an area of the display
  lv_display_set_flush_cb(_display, LvglFlushCallback);

  // raw mode, no io handle...
  // {
  //   LV_LOG_USER("Register io panel event callback for LVGL flush ready notification");
  //   const esp_lcd_panel_io_callbacks_t cbs = {
  //       .on_color_trans_done = LvglFlushReadyCallback,
  //   };

  //   /* Register done callback */
  //   ESP_ERROR_CHECK(esp_lcd_panel_io_register_event_callbacks(io_handle, &cbs, _display));
  //   // lv_display_add_event_cb(_display, lvgl_display_event_cb, LV_EVENT_REFR_READY, NULL);
  // }

  // // associate the mipi panel handle to the display
  // lv_display_set_user_data(_display, panel_handle);

  LV_LOG_USER("Set Color format to RGB565");
  lv_display_set_color_format(_display, LV_COLOR_FORMAT_RGB565);

  return ESP_OK;
};

esp_err_t PanelInit() {
  {
    gpio_set_direction((gpio_num_t)kLcdCsPin, GPIO_MODE_OUTPUT);
    gpio_set_direction((gpio_num_t)kLcdResetPin, GPIO_MODE_OUTPUT);

    PanelReset();
  }

  SetupQSPI();

  {
    // vendor specific intialisation codes
    int i = 1;
    while (i--) {
      const lcd_cmd_t* lcd_init = vendor_config;
      for (int i = 0; i < sizeof(vendor_config) / sizeof(lcd_cmd_t); i++) {
        // amoled_write_cmd(lcd_init[i].addr, lcd_init[i].param, lcd_init[i].len & 0x3f); // < original
        amoled_write_cmd(lcd_init[i].addr, &lcd_init[i].param[0], lcd_init[i].len & 0x3f);
        if (lcd_init[i].len & 0x80) delay(200);
        if (lcd_init[i].len & 0x40) delay(20);
      }
    }
  }

  return ESP_OK;
}

esp_err_t PanelReset() {
  // reset display
  gpio_set_level(kLcdResetPin, 1);
  delay(130);
  gpio_set_level(kLcdResetPin, 0);
  delay(130);
  gpio_set_level(kLcdResetPin, 1);
  delay(300);
  return ESP_OK;
};

esp_err_t LvgLBufferSetupPartial() {
  LV_LOG_USER("Assigning DMA Buffers");

#ifndef CONFIG_SPIRAM
#assert(false);  // Must have PSRAM for LVGL buffers
#endif

  {
    // Allocate persistent framebuffer in PSRAM for rotation
    // _rotationbuffer = (uint8_t*)heap_caps_malloc(kDrawBufferSize, MALLOC_CAP_SPIRAM);
    _rotationbuffer = heap_caps_aligned_alloc(32, kRotationBufferSize, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!_rotationbuffer) {
      LV_LOG_ERROR("Failed to allocate framebuffer in PSRAM");
      return ESP_ERR_NO_MEM;
    }
    LV_LOG_USER("Allocated framebuffer: %p (%zu bytes)", _rotationbuffer, kRotationBufferSize);
  }
  // void* buf1 = spi_bus_dma_memory_alloc(SPI3_HOST, kDrawBufferSize, 0);
  // // void* buf1 = heap_caps_malloc(kDrawBufferSize, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
  // assert(buf1);

  // void* buf2 = spi_bus_dma_memory_alloc(SPI3_HOST, kDrawBufferSize, 0);
  // // void* buf2 = heap_caps_malloc(kDrawBufferSize, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
  // assert(buf2);
  void* buf1 = heap_caps_aligned_alloc(32, kDrawBufferSize, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
  assert(buf1);
  void* buf2 = heap_caps_aligned_alloc(32, kDrawBufferSize, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
  assert(buf2);

  vTaskDelay(10 / portTICK_PERIOD_MS);  // give some time for spi_bus_dma_memory_alloc to settle

  LV_LOG_USER("Allocated DMA buffers: buf1=%p, buf2=%p", buf1, buf2);

  lv_display_set_buffers(_display, buf1, buf2, kDrawBufferSize, LV_DISPLAY_RENDER_MODE_PARTIAL);
  // uint32_t stride = 10;
  // lv_display_set_buffers_with_stride(_display, buf1, buf2, stride, LV_DISPLAY_RENDER_MODE_PARTIAL);

  LV_LOG_USER("Draw buffer size: %zu bytes", kDrawBufferSize);

  // lv_display_set_3rd_draw_buffer() // FIXME: tripple buffers pls!
  return ESP_OK;
}

esp_err_t LvgLBufferSetupFull() {
  LV_LOG_USER("Assigning DMA Buffers");

#ifndef CONFIG_SPIRAM
#assert(false);  // Must have PSRAM for LVGL buffers
#endif

  {
    // Allocate persistent framebuffer in PSRAM for rotation
    // _rotationbuffer = (uint8_t*)heap_caps_malloc(kDrawBufferSize, MALLOC_CAP_SPIRAM);
    _rotationbuffer = heap_caps_aligned_alloc(32, kFramebufferSize, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!_rotationbuffer) {
      LV_LOG_ERROR("Failed to allocate framebuffer in PSRAM");
      return ESP_ERR_NO_MEM;
    }
    LV_LOG_USER("Allocated framebuffer: %p (%zu bytes)", _rotationbuffer, kFramebufferSize);
  }
  // {
  //   // Allocate persistent framebuffer in PSRAM for rotation
  //   _rotationbuffer = (uint8_t*)heap_caps_malloc(kFramebufferSize, MALLOC_CAP_SPIRAM);
  //   if (!_rotationbuffer) {
  //     LV_LOG_ERROR("Failed to allocate framebuffer in PSRAM");
  //     return ESP_ERR_NO_MEM;
  //   }
  //   LV_LOG_USER("Allocated framebuffer: %p (%zu bytes)", _rotationbuffer, kFramebufferSize);
  // }
  // if (!_rotated_buf) {
  //   _rotated_buf = (uint8_t*)heap_caps_malloc(kFramebufferSize, MALLOC_CAP_SPIRAM);
  //   if (!_rotated_buf) {
  //     LV_LOG_ERROR("Failed to allocate rotated_buf in PSRAM");
  //     return ESP_ERR_NO_MEM;
  //   }
  //   LV_LOG_USER("Allocated rotated_buf: %p (%zu bytes)", _rotated_buf, kFramebufferSize);
  // }

  // if (!_contiguous_buf) {
  //   _contiguous_buf = (uint8_t*)heap_caps_malloc(kFramebufferSize, MALLOC_CAP_SPIRAM);
  //   if (!_contiguous_buf) {
  //     LV_LOG_ERROR("Failed to allocate contiguous_buf in PSRAM");
  //     return ESP_ERR_NO_MEM;
  //   }
  //   LV_LOG_USER("Allocated contiguous_buf: %p (%zu bytes)", _contiguous_buf, kFramebufferSize);
  // }

  void* buf1 = heap_caps_aligned_alloc(32, kFramebufferSize, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
  assert(buf1);
  void* buf2 = heap_caps_aligned_alloc(32, kFramebufferSize, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
  assert(buf2);

  vTaskDelay(10 / portTICK_PERIOD_MS);  // give some time for spi_bus_dma_memory_alloc to settle

  LV_LOG_USER("Allocated DMA buffers: buf1=%p, buf2=%p", buf1, buf2);

  // initialize LVGL draw buffers
  lv_display_set_buffers(_display, buf1, buf2, kFramebufferSize, LV_DISPLAY_RENDER_MODE_FULL);
  // lv_display_set_buffers_with_stride(_display, _rotationbuffer, buf1, buf2, LV_DISPLAY_RENDER_MODE_PARTIAL);

  LV_LOG_USER("Draw buffer size: %zu bytes", kDrawBufferSize);
  // lv_display_set_3rd_draw_buffer() // FIXME: tripple buffers pls!
  return ESP_OK;
}

esp_err_t TouchPanelSetup() {
  LV_LOG_USER("Setting up touch panel");

  i2c_device_config_t i2c_dev_conf = {
      .dev_addr_length = I2C_ADDR_BIT_LEN_7,
      .device_address = kTouchI2cAddress,
      .scl_speed_hz = I2cManager::kClockSpeedHz,
  };
  esp_err_t err = I2cManager::GetInstance()->AddDevice(&i2c_dev_conf, &_dev_handle);
  if (err != ESP_OK) {
    FLOG_ERROR("Failed to add touch device: %s", esp_err_to_name(err));
    return err;
  }
  static lv_indev_t* indev;
  indev = lv_indev_create();  // Input device driver (SetupTouchPanel)
  lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
  lv_indev_set_display(indev, _display);
  // lv_indev_set_user_data(indev, tp);

  lv_indev_set_read_cb(indev, LvglTouchCallback);
  return ESP_OK;
}

void GetDisplayDimensions(uint16_t& width, uint16_t& height) {
  width = kHRes;
  height = kVRes;
}

lv_display_t* GetDisplayObjPtr() { return _display; }

esp_err_t SetupQSPI() {
  spi_bus_config_t buscfg = {
      .data0_io_num = kLcdData0Pin,
      .data1_io_num = kLcdData1Pin,
      .sclk_io_num = kLcdSckPin,
      .data2_io_num = kLcdData2Pin,
      .data3_io_num = kLcdData3Pin,
      .data4_io_num = GPIO_NUM_NC,
      .data5_io_num = GPIO_NUM_NC,
      .data6_io_num = GPIO_NUM_NC,
      .data7_io_num = GPIO_NUM_NC,
      .max_transfer_sz = kMaxTransferSize,  //(SEND_BUF_SIZE * 16) + 8,
      .flags = SPICOMMON_BUSFLAG_MASTER | SPICOMMON_BUSFLAG_GPIO_PINS,
  };

  spi_device_interface_config_t devcfg = {
      .command_bits = 8,
      .address_bits = 24,
      .mode = 0,
      .clock_speed_hz = kLcdPixelClockHz,  // DEFAULT_SCK_SPEED,
      .spics_io_num = -1,
      .flags = SPI_DEVICE_HALFDUPLEX,
      .queue_size = 17,  //<! 17 is vendor value
  };
  ESP_ERROR_CHECK(spi_bus_initialize(SPI3_HOST, &buscfg, SPI_DMA_CH_AUTO));

  ESP_ERROR_CHECK(spi_bus_add_device(SPI3_HOST, &devcfg, &_spi));

  return ESP_OK;
};

// void LvglFlushCallback(lv_display_t* disp, const lv_area_t* area, uint8_t* px_map) {
//   LV_LOG_INFO("Flush......");
//   lv_display_rotation_t rotation = lv_display_get_rotation(disp);
//   lv_area_t rotated_area;

//   lv_draw_sw_rgb565_swap(px_map, (area->x2 + 1 - area->x1) * (area->y2 + 1 - area->y1));

//   if (rotation != LV_DISPLAY_ROTATION_0) {
//     memset(_rotationbuffer, 0, sizeof(_rotationbuffer));
//     lv_color_format_t cf = lv_display_get_color_format(disp);
//     /*Calculate the position of the rotated area*/
//     rotated_area = *area;
//     lv_display_rotate_area(disp, &rotated_area);
//     /*Calculate the source stride (bytes in a line) from the width of the area*/
//     uint32_t src_stride = lv_draw_buf_width_to_stride(lv_area_get_width(area), cf);
//     /*Calculate the stride of the destination (rotated) area too*/
//     uint32_t dest_stride = lv_draw_buf_width_to_stride(lv_area_get_width(&rotated_area), cf);
//     /*Have a buffer to store the rotated area and perform the rotation*/
//     // static uint8_t rotated_buf[kDrawBufferSize];
//     int32_t src_w = lv_area_get_width(area);
//     int32_t src_h = lv_area_get_height(area);
//     // lv_draw_sw_rotate(px_map, rotated_buf, src_w, src_h, src_stride, dest_stride, rotation, cf);
//     lv_draw_sw_rotate(px_map, _rotationbuffer, src_w, src_h, src_stride, dest_stride, rotation, cf);

//     /*Use the rotated area and rotated buffer from now on*/
//     area = &rotated_area;
//     // px_map = rotated_buf;
//     px_map = _rotationbuffer;
//   }
//   display_push_colors(rotated_area.x1, rotated_area.y1, lv_area_get_width(area), lv_area_get_height(area),
//                       (uint16_t*)px_map);

//   // my_set_window(area->x1, area->y1, area->x2, area->y2);
//   // my_send_colors(px_map);
//   lv_display_flush_ready(disp);
// }

// void LvglFlushCallback(lv_display_t* disp, const lv_area_t* area, uint8_t* px_map) {
//   LV_LOG_INFO("Flush......");
//   lv_color_format_t cf = lv_display_get_color_format(disp);
//   lv_display_rotation_t rotation = lv_display_get_rotation(disp);

//   lv_draw_sw_rgb565_swap(px_map, (area->x2 + 1 - area->x1) * (area->y2 + 1 - area->y1));

//   if (rotation != LV_DISPLAY_ROTATION_0) {
//     lv_area_t rotated_area = *area;
//     lv_display_rotate_area(disp, &rotated_area);

//     int32_t src_w = lv_area_get_width(area);
//     int32_t src_h = lv_area_get_height(area);
//     int32_t rotated_w = lv_area_get_width(&rotated_area);
//     int32_t rotated_h = lv_area_get_height(&rotated_area);

//     uint32_t src_stride = lv_draw_buf_width_to_stride(src_w, cf);
//     uint32_t dest_stride = lv_draw_buf_width_to_stride(rotated_w, cf);

//     lv_draw_sw_rotate(px_map, _rotationbuffer, src_w, src_h, src_stride, dest_stride, rotation, cf);

//     // De-stride: copy rotated data from strided buffer into contiguous buffer
//     uint8_t* contiguous_buf = _rotationbuffer + (kFramebufferSize / 2);  // Use second half of framebuffer
//     uint8_t* src = _rotationbuffer;
//     uint8_t* dst = contiguous_buf;
//     size_t pixel_size = sizeof(lv_color16_t);

//     for (int32_t y = 0; y < rotated_h; y++) {
//       memcpy(dst, src, rotated_w * pixel_size);
//       src += dest_stride;
//       dst += rotated_w * pixel_size;
//     }
//     area = &rotated_area;
//     px_map = contiguous_buf;
//   }
//   // display_push_colors(rotated_area.x1, rotated_area.y1, rotated_w, rotated_h, (uint16_t*)contiguous_buf);
//   display_push_colors(area->x1, area->y1, lv_area_get_width(area), lv_area_get_height(area), (uint16_t*)px_map);

//   if (lv_display_flush_is_last(_display)) {
//     LV_LOG_INFO("LVGL frame rendered");
//   }

//   lv_display_flush_ready(disp);
// }

// void LvglFlushCallbackFullFrameTry(lv_display_t* disp, const lv_area_t* area, uint8_t* px_map) {
//   LV_LOG_INFO("Flush x1=%li y1=%li x2=%li y2=%li w=%d h=%d", area->x1, area->y1, area->x2, area->y2,
//             (int)lv_area_get_width(area), (int)lv_area_get_height(area));
//   lv_color_format_t cf = lv_display_get_color_format(disp);
//   lv_display_rotation_t rotation = lv_display_get_rotation(disp);

//   // lv_draw_sw_rgb565_swap(px_map, (area->x2 + 1 - area->x1) * (area->y2 + 1 - area->y1));

//   if (rotation != LV_DISPLAY_ROTATION_0) {
//     lv_area_t rotated_area = *area;
//     lv_display_rotate_area(disp, &rotated_area);

//     int32_t src_w = lv_area_get_width(area);
//     int32_t src_h = lv_area_get_height(area);
//     int32_t rotated_w = lv_area_get_width(&rotated_area);
//     int32_t rotated_h = lv_area_get_height(&rotated_area);

//     uint32_t src_stride = lv_draw_buf_width_to_stride(src_w, cf);

//     // Rotate directly to _rotationbuffer (contiguously, no stride)
//     lv_draw_sw_rotate(px_map, _rotationbuffer, src_w, src_h, src_stride, rotated_w * sizeof(lv_color16_t), rotation,
//     cf);

//     // Send FULL display using original dimensions (180×640), not rotated area
//     display_push_colors(0, 0, kHRes, kVRes, (uint16_t*)_rotationbuffer);
//         display_push_colors(area->x1, area->y1, lv_area_get_width(area), lv_area_get_height(area),
//         (uint16_t*)px_map);

//   } else {
//     // display_push_colors(0, 0, kHRes, kVRes, (uint16_t*)_rotationbuffer);

//     display_push_colors(area->x1, area->y1, lv_area_get_width(area), lv_area_get_height(area), (uint16_t*)px_map);
//   }

//   if (lv_display_flush_is_last(_display)) {
//     LV_LOG_INFO("LVGL frame rendered");
//   }

//   lv_display_flush_ready(disp);
// }

// void LvglFlushCallback(lv_display_t* disp, const lv_area_t* area, uint8_t* color_map) {
//   const int lv_w = 640;
//   const int lv_h = 180;

//   int w = area->x2 - area->x1 + 1;
//   int h = area->y2 - area->y1 + 1;

//   // Rotate buffer output: needs w*h*2 bytes
//   static uint16_t* rot = NULL;
//   static size_t rot_size = 0;

//   size_t need = w * h * sizeof(uint16_t);
//   if (need > rot_size) {
//     if (rot) free(rot);
//     rot = malloc(need);
//     rot_size = need;
//   }

//   uint16_t* src = (uint16_t*)color_map;
//   uint16_t* dst = rot;

//   // Rotate 90° CCW: panel(x,y) = LVGL(y, W-1-x)
//   for (int yy = 0; yy < h; yy++) {
//     for (int xx = 0; xx < w; xx++) {
//       int lv_x = area->x1 + xx;
//       int lv_y = area->y1 + yy;

//       int panel_x = lv_y;
//       int panel_y = (lv_w - 1) - lv_x;

//       dst[yy * w + xx] = src[(lv_y - area->y1) * w + (lv_x - area->x1)];
//     }
//   }

//   // Send rotated data using panel coordinate space
//   amoled_set_window(panel_x_start, panel_x_end, panel_y_start, panel_y_end);

//   amoled_push_buffer(rot, w * h * 2);

//   lv_display_flush_ready(disp);
// }

// void LvglFlushCallback_another_bs_one(lv_display_t* disp, const lv_area_t* area, uint8_t* px_map) {
//   const int LV_W = 640;  // LVGL horizontal resolution (landscape)
//   // area is in LVGL coords (0..639, 0..179)
//   int32_t x1 = area->x1;
//   int32_t y1 = area->y1;
//   int32_t x2 = area->x2;
//   int32_t y2 = area->y2;

//   int32_t w = x2 - x1 + 1;
//   int32_t h = y2 - y1 + 1;
//   if (w <= 0 || h <= 0) {
//     lv_display_flush_ready(disp);
//     return;
//   }

//   // Color bytes are already swapped earlier in your code; px_map is RGB565 packed (uint16_t per pixel)
//   uint16_t* src = (uint16_t*)px_map;
//   size_t npixels = (size_t)w * (size_t)h;

//   // allocate or reuse rotation buffer (store in PSRAM ideally)
//   static uint16_t* rot_buf = NULL;
//   static size_t rot_buf_pixels = 0;
//   if (npixels > rot_buf_pixels) {
//     if (rot_buf) {
//       free(rot_buf);
//       rot_buf = NULL;
//       rot_buf_pixels = 0;
//     }
//     rot_buf = (uint16_t*)heap_caps_malloc(npixels * sizeof(uint16_t), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
//     if (!rot_buf) {
//       // fallback to heap
//       rot_buf = (uint16_t*)malloc(npixels * sizeof(uint16_t));
//     }
//     if (!rot_buf) {
//       lv_display_flush_ready(disp);
//       return;
//     }
//     rot_buf_pixels = npixels;
//   }

//   // Panel coordinate mapping:
//   // panel_x ranges [0..179]  <- LV y
//   // panel_y ranges [0..639]  <- LV x mapped as panel_y = (LV_W - 1) - LV_x
//   // Target window in panel coords:
//   uint16_t panel_x_start = (uint16_t)y1;
//   uint16_t panel_x_end = (uint16_t)y2;
//   uint16_t panel_y_start = (uint16_t)(LV_W - 1 - x2);
//   uint16_t panel_y_end = (uint16_t)(LV_W - 1 - x1);

//   // Fill rot_buf in panel row-major order: for panel_y = panel_y_start..panel_y_end (rows),
//   // for panel_x = panel_x_start..panel_x_end (cols), push pixel(panel_x, panel_y)
//   // Reverse-map to LV coords: LV_x = (LV_W - 1) - panel_y; LV_y = panel_x
//   size_t di = 0;
//   for (uint32_t py = panel_y_start; py <= panel_y_end; ++py) {
//     for (uint32_t px = panel_x_start; px <= panel_x_end; ++px) {
//       int32_t lv_x = (LV_W - 1) - (int32_t)py;
//       int32_t lv_y = (int32_t)px;
//       // compute src offset inside the LV area
//       int32_t sx = lv_x - x1;
//       int32_t sy = lv_y - y1;
//       uint16_t pix = 0x0000;
//       if ((sx >= 0) && (sx < w) && (sy >= 0) && (sy < h)) {
//         pix = src[sy * w + sx];
//       }
//       rot_buf[di++] = pix;
//     }
//   }

//   // Set the panel window in panel coordinates and push rotated buffer
//   amoled_set_window(panel_x_start, panel_x_end, panel_y_start, panel_y_end);
//   amoled_push_buffer(rot_buf, (uint32_t)npixels);

//   lv_display_flush_ready(disp);
// }

// void LvglFlushCallback(lv_display_t* disp, const lv_area_t* area, uint8_t* px_map) {
//   lv_draw_sw_rgb565_swap(px_map, kHRes * kVRes);

//   amoled_set_window(0, 0, 179, 639);
//   amoled_push_buffer((uint16_t*)px_map, 180 * 640);

//   lv_display_flush_ready(disp);
// }
// void LvglFlushCallback_working?(lv_display_t* disp, const lv_area_t* area, uint8_t* px_map) {
//   lv_display_rotation_t rotation = lv_display_get_rotation(disp);
//   lv_area_t rotated_area;
//   lv_draw_sw_rgb565_swap(px_map, (area->x2 + 1 - area->x1) * (area->y2 + 1 - area->y1));

//   if (rotation != LV_DISPLAY_ROTATION_0) {
//     lv_color_format_t cf = lv_display_get_color_format(disp);

//     // Calculate the position of the rotated area
//     rotated_area = *area;
//     lv_display_rotate_area(disp, &rotated_area);

//     // Calculate stride for source and destination
//     uint32_t src_stride = lv_draw_buf_width_to_stride(lv_area_get_width(area), cf);
//     uint32_t dest_stride = lv_draw_buf_width_to_stride(lv_area_get_width(&rotated_area), cf);

//     // Buffer to store the rotated area (WITH stride)
//     static uint8_t rotated_buf[kFramebufferSize];
//     int32_t src_w = lv_area_get_width(area);
//     int32_t src_h = lv_area_get_height(area);

//     // Perform rotation (this creates STRIDED data)
//     lv_draw_sw_rotate(px_map, rotated_buf, src_w, src_h, src_stride, dest_stride, rotation, cf);

//     // De-stride: copy rotated data from strided buffer into contiguous form
//     int32_t rotated_w = lv_area_get_width(&rotated_area);
//     int32_t rotated_h = lv_area_get_height(&rotated_area);
//     static uint8_t contiguous_buf[kFramebufferSize];

//     size_t bytes_per_pixel = 2;                      // RGB565 = 2 bytes per pixel
//     size_t row_bytes = rotated_w * bytes_per_pixel;  // Actual data per row

//     LV_LOG_INFO("De-striding: w=%ld h=%ld row_bytes=%zu dest_stride=%lu", rotated_w, rotated_h, row_bytes,
//     dest_stride);

//     // Copy each row from strided buffer to contiguous buffer
//     uint8_t* src = rotated_buf;
//     uint8_t* dst = contiguous_buf;
//     for (int32_t y = 0; y < rotated_h; y++) {
//       memcpy(dst, src, row_bytes);
//       src += dest_stride;  // Skip to next row (including padding)
//       dst += row_bytes;    // Next row in contiguous buffer (no padding)
//     }

//     // Use the rotated area and de-strided contiguous buffer
//     area = &rotated_area;
//     px_map = contiguous_buf;  // Use de-strided data

//     LV_LOG_INFO("Rotating: src_w=%ld src_h=%ld src_stride=%lu dest_stride=%lu", src_w, src_h, src_stride,
//     dest_stride);
//   }

//   LV_LOG_INFO("Sending to display - window: x1=%li y1=%li x2=%li y2=%li, pixels=%ld", area->x1, area->y1, area->x2,
//             area->y2, (area->x2 + 1 - area->x1) * (area->y2 + 1 - area->y1));

//   amoled_set_window(area->x1, area->y1, area->x2, area->y2);
//   amoled_push_buffer((uint16_t*)px_map, (area->x2 + 1 - area->x1) * (area->y2 + 1 - area->y1));

//   lv_display_flush_ready(disp);
// }

/*Rotate a partially rendered area to another buffer and send it*/
void LvglFlushCallback(lv_display_t* disp, const lv_area_t* area, uint8_t* px_map) {
  lv_display_rotation_t rotation = lv_display_get_rotation(disp);
  lv_area_t rotated_area;
  if (rotation != LV_DISPLAY_ROTATION_0) {
    lv_color_format_t cf = lv_display_get_color_format(disp);
    /*Calculate the position of the rotated area*/
    rotated_area = *area;
    lv_display_rotate_area(disp, &rotated_area);
    /*Calculate the source stride (bytes in a line) from the width of the area*/
    uint32_t src_stride = lv_draw_buf_width_to_stride(lv_area_get_width(area), cf);
    /*Calculate the stride of the destination (rotated) area too*/
    uint32_t dest_stride = lv_draw_buf_width_to_stride(lv_area_get_width(&rotated_area), cf);
    /*Have a buffer to store the rotated area and perform the rotation*/
    int32_t src_w = lv_area_get_width(area);
    int32_t src_h = lv_area_get_height(area);
    lv_draw_sw_rotate(px_map, _rotationbuffer, src_w, src_h, src_stride, dest_stride, rotation, cf);
    /*Use the rotated area and rotated buffer from now on*/
    area = &rotated_area;
    px_map = reinterpret_cast<uint8_t*>(_rotationbuffer);
  }
  LV_LOG_INFO("widht=%ld, height=%ld, ", (area->x2 + 1 - area->x1), (area->y2 + 1 - area->y1));
  amoled_set_window(area->x1, area->y1, area->x2, area->y2);
  amoled_push_buffer((uint16_t*)px_map, (area->x2 + 1 - area->x1) * (area->y2 + 1 - area->y1));
  lv_display_flush_ready(disp);
}

// void LvglFlushCallback_afterbob(lv_display_t* disp, const lv_area_t* area, uint8_t* px_map) {
//   lv_display_rotation_t rotation = lv_display_get_rotation(disp);
//   lv_area_t rotated_area;
//   // lv_draw_sw_rgb565_swap(px_map, (area->x2 + 1 - area->x1) * (area->y2 + 1 - area->y1));
//   if (rotation != LV_DISPLAY_ROTATION_0) {
//     lv_color_format_t cf = lv_display_get_color_format(disp);
//     /*Calculate the position of the rotated area*/
//     rotated_area = *area;
//     lv_display_rotate_area(disp, &rotated_area);
//     /*Calculate the source stride (bytes in a line) from the width of the area*/
//     uint32_t src_stride = lv_draw_buf_width_to_stride(lv_area_get_width(area), cf);
//     /*Calculate the stride of the destination (rotated) area too*/
//     uint32_t dest_stride = lv_draw_buf_width_to_stride(lv_area_get_width(&rotated_area), cf);
//     /*Have a buffer to store the rotated area and perform the rotation*/
//     static uint8_t rotated_buf[kDrawBufferSize];
//     int32_t src_w = lv_area_get_width(area);
//     int32_t src_h = lv_area_get_height(area);
//     // memset(rotated_buf, 0, kDrawBufferSize);  // Clear before rotation

//     lv_draw_sw_rotate(px_map, rotated_buf, src_w, src_h, src_stride, dest_stride, rotation, cf);
//     area = &rotated_area;
//     px_map = rotated_buf;

//     // LV_LOG_INFO("src_stride=%lu, dest_stride=%lu, rotated_h=%ld, total_needed=%ld, buffer_size=%zu", src_stride,
//     //           dest_stride, lv_area_get_height(&rotated_area), lv_area_get_height(&rotated_area) * dest_stride,
//     //           kDrawBufferSize);

//     LV_LOG_INFO("Rotating: src_w=%ld src_h=%ld src_stride=%lu dest_stride=%lu", src_w, src_h, src_stride,
//     dest_stride);
//     // LV_LOG_INFO("Before rotation - area: x1=%li y1=%li x2=%li y2=%li", area->x1, area->y1, area->x2, area->y2);
//     // LV_LOG_INFO("After rotation - rotated_area: x1=%li y1=%li x2=%li y2=%li", rotated_area.x1, rotated_area.y1,
//     // rotated_area.x2, rotated_area.y2);
//   }
//   LV_LOG_INFO("Sending to display - window: x1=%li y1=%li x2=%li y2=%li, pixels=%ld", area->x1, area->y1, area->x2,
//             area->y2, (area->x2 + 1 - area->x1) * (area->y2 + 1 - area->y1));

//   amoled_set_window(area->x1, area->y1, area->x2, area->y2);
//   amoled_push_buffer((uint16_t*)px_map, (area->x2 + 1 - area->x1) * (area->y2 + 1 - area->y1));

//   // vTaskDelay(100 / portTICK_PERIOD_MS);  // Small delay to allow SPI to settle
//   lv_display_flush_ready(disp);
// }

// copilots not working attempt
// void LvglFlushCallback(lv_display_t* disp, const lv_area_t* area, uint8_t* px_map) {
//   lv_display_rotation_t rotation = lv_display_get_rotation(disp);
//   lv_area_t rotated_area;
//   lv_draw_sw_rgb565_swap(px_map, (area->x2 + 1 - area->x1) * (area->y2 + 1 - area->y1));
//   if (rotation != LV_DISPLAY_ROTATION_0) {
//     lv_color_format_t cf = lv_display_get_color_format(disp);
//     /*Calculate the position of the rotated area*/
//     rotated_area = *area;
//     lv_display_rotate_area(disp, &rotated_area);
//     /*Calculate the source stride (bytes in a line) from the width of the area*/
//     uint32_t src_stride = lv_draw_buf_width_to_stride(lv_area_get_width(area), cf);
//     /*Calculate the stride of the destination (rotated) area too*/
//     uint32_t dest_stride = lv_draw_buf_width_to_stride(lv_area_get_width(&rotated_area), cf);
//     /*Have a buffer to store the rotated area and perform the rotation*/
//     static uint8_t rotated_buf[kDrawBufferSize];
//     int32_t src_w = lv_area_get_width(area);
//     int32_t src_h = lv_area_get_height(area);

//     lv_draw_sw_rotate(px_map, rotated_buf, src_w, src_h, src_stride, dest_stride, rotation, cf);

//     // De-stride: copy rotated data from strided buffer into contiguous form
//     int32_t rotated_w = lv_area_get_width(&rotated_area);
//     int32_t rotated_h = lv_area_get_height(&rotated_area);
//     static uint8_t contiguous_buf[kDrawBufferSize];
//     uint8_t* src = rotated_buf;
//     uint8_t* dst = contiguous_buf;
//     size_t pixel_size = sizeof(lv_color16_t);

//     size_t row_bytes = rotated_w * pixel_size;

//     // LV_LOG_INFO("De-stride: rotated_w=%ld rotated_h=%ld row_bytes=%zu dest_stride=%lu", rotated_w, rotated_h,
//     // row_bytes,dest_stride);

//     // for (int32_t y = 0; y < rotated_h; y++) {
//     //   memcpy(dst, src, rotated_w * pixel_size);
//     //   src += dest_stride;
//     //   dst += rotated_w * pixel_size;
//     // }

//     /*Use the rotated area and rotated buffer from now on*/
//     area = &rotated_area;
//     // px_map = contiguous_buf;  // Use de-strided data instead of rotated_buf
//     px_map = rotated_buf;  // Use rotated_buf directly (with stride)
//     // LV_LOG_INFO("Rotating: src_w=%ld src_h=%ld src_stride=%lu dest_stride=%lu", src_w, src_h, src_stride,
//     // dest_stride); LV_LOG_INFO("Before rotation - area: x1=%li y1=%li x2=%li y2=%li", area->x1, area->y1,
//     // area->x2, area->y2); LV_LOG_INFO("After rotation - rotated_area: x1=%li y1=%li x2=%li y2=%li",
//     // rotated_area.x1, rotated_area.y1,rotated_area.x2, rotated_area.y2);
//   }
//   LV_LOG_INFO("Sending to display - window: x1=%li y1=%li x2=%li y2=%li, pixels=%ld", area->x1, area->y1, area->x2,
//             area->y2, (area->x2 + 1 - area->x1) * (area->y2 + 1 - area->y1));

//   amoled_set_window(area->x1, area->y1, area->x2, area->y2);
//   amoled_push_buffer((uint16_t*)px_map, (area->x2 + 1 - area->x1) * (area->y2 + 1 - area->y1));

//   lv_display_flush_ready(disp);
// }

// void LvglFlushCallback_bestest(lv_display_t* disp, const lv_area_t* area, uint8_t* px_map) {
//   lv_display_rotation_t rotation = lv_display_get_rotation(disp);
//   lv_area_t rotated_area;
//   lv_draw_sw_rgb565_swap(px_map, (area->x2 + 1 - area->x1) * (area->y2 + 1 - area->y1));
//   if (rotation != LV_DISPLAY_ROTATION_0) {
//     lv_color_format_t cf = lv_display_get_color_format(disp);
//     /*Calculate the position of the rotated area*/
//     rotated_area = *area;
//     lv_display_rotate_area(disp, &rotated_area);
//     /*Calculate the source stride (bytes in a line) from the width of the area*/
//     uint32_t src_stride = lv_draw_buf_width_to_stride(lv_area_get_width(area), cf);
//     /*Calculate the stride of the destination (rotated) area too*/
//     uint32_t dest_stride = lv_draw_buf_width_to_stride(lv_area_get_width(&rotated_area), cf);
//     /*Have a buffer to store the rotated area and perform the rotation*/
//     static uint8_t rotated_buf[kDrawBufferSize];
//     int32_t src_w = lv_area_get_width(area);
//     int32_t src_h = lv_area_get_height(area);

//     lv_draw_sw_rotate(px_map, rotated_buf, src_w, src_h, src_stride, dest_stride, rotation, cf);
//     // lv_draw_sw_rotate(px_map, rotated_buf, src_w, src_h, 0, 0, rotation, cf);
//     /*Use the rotated area and rotated buffer from now on*/
//     area = &rotated_area;
//     px_map = rotated_buf;
//     LV_LOG_INFO("Rotating: src_w=%ld src_h=%ld src_stride=%lu dest_stride=%lu", src_w, src_h, src_stride,
//     dest_stride); LV_LOG_INFO("Before rotation - area: x1=%li y1=%li x2=%li y2=%li", area->x1, area->y1, area->x2,
//     area->y2); LV_LOG_INFO("After rotation - rotated_area: x1=%li y1=%li x2=%li y2=%li", rotated_area.x1,
//     rotated_area.y1,
//               rotated_area.x2, rotated_area.y2);
//   }
//   LV_LOG_INFO("Sending to display - window: x1=%li y1=%li x2=%li y2=%li, pixels=%ld", area->x1, area->y1, area->x2,
//             area->y2, (area->x2 + 1 - area->x1) * (area->y2 + 1 - area->y1));

//   // display_push_colors(area->x1, area->y1, area->x2, area->y2, (uint16_t*)px_map);
//   // display_push_colors(area->x1, area->y1, lv_area_get_width(area), lv_area_get_height(area), (uint16_t*)px_map);
//   // display_push_colors(area->x1, area->y1, area->x2, area->y2 - 1, (uint16_t*)px_map);

//   amoled_set_window(area->x1, area->y1, area->x2, area->y2);
//   amoled_push_buffer((uint16_t*)px_map, (area->x2 + 1 - area->x1) * (area->y2 + 1 - area->y1));
//   // (area->x2 + 1 - area->x1) * (area->y2 + 1 - area->y1));  // lv_area_get_width(area) * lv_area_get_height(area));

//   // display_push_colors(rotated_area.x1, rotated_area.y1, lv_area_get_width(&rotated_area),
//   // lv_area_get_height(&rotated_area), (uint16_t*)px_map);
//   // my_set_window(area->x1, area->y1, area->x2, area->y2);
//   // my_send_colors(px_map);

//   lv_display_flush_ready(disp);
// }

bool LvglFlushReadyCallback(esp_lcd_panel_io_handle_t io, esp_lcd_panel_io_event_data_t* edata, void* user_data) {
  LV_LOG_USER("LVGL Flush Ready Callback????????????????????????????");
  lv_display_t* disp = (lv_display_t*)user_data;
  lv_display_flush_ready(disp);
  return false;
};

uint8_t TouchGetData(int16_t* x, int16_t* y, uint8_t point_num) {
  if (!_dev_handle) {
    return 0;  // Device not initialized
  }

  uint8_t buffer[20] = {0};
  uint8_t cmd[11] = {0xb5, 0xab, 0xa5, 0x5a, 0x0, 0x0, 0x0, 0x8, 0x0, 0x0, 0x0};

  // Use transmit_receive directly to avoid separate Write+Read calls
  esp_err_t err = i2c_master_transmit_receive(_dev_handle, cmd, sizeof(cmd), buffer, 20, -1);
  if (err != ESP_OK) {
    return 0;  // Silently fail
  }

  uint16_t type = AXS_GET_GESTURE_TYPE(buffer);
  uint16_t pointX = AXS_GET_POINT_X(buffer, 0);
  uint16_t pointY = AXS_GET_POINT_Y(buffer, 0);

  if (!type && (pointX || pointY)) {
    *x = pointY;
    *y = 640 - pointX;
    return 1;
  }
  return 0;
}

// uint8_t TouchGetData(int16_t* x, int16_t* y, uint8_t point_num) {
//   uint8_t buffer[20] = {0};

//   // Send unlock command first
//   uint8_t cmd[11] = {0xb5, 0xab, 0xa5, 0x5a, 0x0, 0x0, 0x0, 0x8, 0x0, 0x0, 0x0};
//   esp_err_t err = I2cManager::GetInstance()->Write(_dev_handle, cmd, sizeof(cmd));
//   if (err != ESP_OK) {
//     LV_LOG_ERROR("Touch unlock failed: %s", esp_err_to_name(err));
//     return 0;
//   }

//   // Now read the touch data
//   err = I2cManager::GetInstance()->Read(_dev_handle, buffer, 20);
//   if (err != ESP_OK) {
//     LV_LOG_ERROR("Touch read failed: %s", esp_err_to_name(err));
//     return 0;
//   }

//   uint16_t type = AXS_GET_GESTURE_TYPE(buffer);
//   uint16_t pointX = AXS_GET_POINT_X(buffer, 0);
//   uint16_t pointY = AXS_GET_POINT_Y(buffer, 0);

//   if (!type && (pointX || pointY)) {
//     *x = pointY;
//     *y = 640 - pointX;
//     LV_LOG_USER("Touch: X=%d Y=%d", *x, *y);
//     return 1;
//   }
//   return 0;
// }

// uint8_t TouchGetData(int16_t* x, int16_t* y, uint8_t point_num) {
//   uint8_t touched = 0;
//   uint16_t pointX;
//   uint16_t pointY;
//   uint16_t type = 0;

//   uint8_t cmd[11] = {0xb5, 0xab, 0xa5, 0x5a, 0x0, 0x0, 0x0, 0x8};
//   uint8_t buffer[20] = {0};

//   // I2cManager::GetInstance()->ReadRegister(_dev_handle, 0x00, buffer, 20);

//   if (ESP_OK != i2c_master_transmit_receive(_dev_handle, cmd, sizeof(cmd) / sizeof(*cmd), buffer, 20, -1)) {
//     return 0;
//   }

//   type = AXS_GET_GESTURE_TYPE(buffer);
//   pointX = AXS_GET_POINT_X(buffer, 0);
//   pointY = AXS_GET_POINT_Y(buffer, 0);

//   if (!type && (pointX || pointY)) {
//     *x = pointY;
//     *y = 640 - pointX;
//     LV_LOG_USER("T:%d X:%d Y:%d", type, *x, *y);
//     touched = 1;
//   }
//   return touched;

// }

void LvglTouchCallback(lv_indev_t* indev, lv_indev_data_t* data) {
  // void LvglTouchCallback(esp_lcd_touch_handle_t tp) {
  int16_t touchpad_x[1] = {0};
  int16_t touchpad_y[1] = {0};
  uint8_t touchpad_cnt = 0;

  /* Get coordinates */
  touchpad_cnt = TouchGetData(touchpad_x, touchpad_y, 1);

  if (touchpad_cnt > 0) {
    data->point.x = touchpad_x[0];
    data->point.y = touchpad_y[0];
    data->state = LV_INDEV_STATE_PRESSED;
  } else {
    data->state = LV_INDEV_STATE_RELEASED;
  }
  LV_LOG_INFO("Touch data: x=%li y=%li state=%d", data->point.x, data->point.y, data->state);
}

void TurnOn() {
  lv_async_call(
      [](void*) {
        // TestPanelGeometry();
        ShowBootScreen();
        Backlight();
      },
      nullptr);
};

void ShowBootScreen() {
  LV_LOG_USER("Displaying boot screen");
  // return;

  lv_obj_t* boot_scr = lv_obj_create(NULL);
  lv_obj_set_style_bg_color(boot_scr, lv_color_black(), 0);
  lv_obj_set_style_bg_opa(boot_scr, LV_OPA_COVER, 0);

  // lv_obj_t* obj = lv_obj_create(boot_scr);
  // lv_obj_center(obj);

  lv_obj_t* label = lv_label_create(boot_scr);
  lv_label_set_text(label, "Toothless");
  lv_obj_set_style_text_font(label, &lv_font_montserrat_32, 0);
  lv_obj_center(label);

  lv_obj_t* sub_label = lv_label_create(boot_scr);
  lv_label_set_text(sub_label, "Initializing...");
  lv_obj_set_style_text_font(sub_label, &lv_font_montserrat_20, 0);
  lv_obj_align_to(sub_label, label, LV_ALIGN_OUT_BOTTOM_MID, 0, 10);
  lv_screen_load(boot_scr);

  // lv_obj_t* boot_scr = lv_obj_create(NULL);
  // lv_obj_set_style_bg_color(boot_scr, lv_color_black(), 0);
  // // lv_obj_set_style_bg_color(boot_scr, lv_color_hex(0xFF00FF), 0);

  // lv_obj_set_style_bg_opa(boot_scr, LV_OPA_COVER, 0);

  // lv_obj_t* label = lv_label_create(boot_scr);
  // lv_label_set_text(label, "Toothless");
  // lv_obj_set_style_text_font(label, &lv_font_montserrat_32, 0);
  // lv_obj_center(label);

  // lv_obj_t* sub_label = lv_label_create(boot_scr);
  // lv_label_set_text(sub_label, "Initializing...");
  // lv_obj_set_style_text_font(sub_label, &lv_font_montserrat_20, 0);
  // lv_obj_align_to(sub_label, label, LV_ALIGN_OUT_BOTTOM_MID, 0, 10);

  // lv_display_flush_ready(_display);  // trigger LVGL flush

  // lv_screen_load(boot_scr);
  // Backlight();
}

static constexpr size_t kPixelSizeBytes = 2;

static constexpr uint16_t kBlue = 0x001f;
static constexpr uint16_t kDarkBlue = 0x0010;
static constexpr uint16_t kRed = 0xf800;
static constexpr uint16_t kDarkRed = 0x8000;
static constexpr uint16_t kGreen = 0x07e0;
static constexpr uint16_t kDarkGreen = 0x0400;
static constexpr uint16_t kPurple = kBlue | kRed;
static constexpr uint16_t kDarkPurple = kDarkBlue | kDarkRed;
static constexpr uint16_t kYellow = kGreen | kRed;
static constexpr uint16_t kDarkYellow = kDarkGreen | kDarkRed;
static constexpr uint16_t kCyan = kBlue | kGreen;
static constexpr uint16_t kDarkCyan = kDarkBlue | kDarkGreen;
static constexpr uint16_t kBlack = 0x0000;
static constexpr uint16_t kWhite = kBlue | kGreen | kRed;

static constexpr size_t kScreenWidth = 180;
static constexpr size_t kScreenHeight = 640;
static constexpr size_t kPadding = (0x1 << 5);
static constexpr size_t kLowBits = kPadding - 1;
static constexpr size_t kHighBits = ~kLowBits;
static constexpr size_t kPaddedScreenWidth = 180;  // kScreenWidth;
//      (kScreenWidth & kLowBits) == 0 ? kScreenWidth : (kScreenWidth & kHighBits) + kPadding;
static constexpr size_t kPaddedScreenHeight = 640;  // kScreenHeight;
//      (kScreenHeight & kLowBits) == 0 ? kScreenHeight : (kScreenHeight & kHighBits) + kPadding;
static constexpr size_t kFramebufferSizePixels = kPaddedScreenWidth * kPaddedScreenHeight;
static constexpr size_t kFramebufferSizeBytes = kFramebufferSizePixels * kPixelSizeBytes;

void clearScreen(uint16_t* buf) {
  for (size_t x = 0; x < kScreenWidth; x++) {
    for (size_t y = 0; y < kScreenHeight; y++) {
      buf[y * kPaddedScreenWidth + x] = kBlack;
    }
  }

  for (size_t x = kScreenWidth; x < kPaddedScreenWidth; ++x) {
    for (size_t y = 0; y < kPaddedScreenHeight; ++y) {
      buf[y * kPaddedScreenWidth + x] = kDarkPurple;
    }
  }
  for (size_t x = 0; x < kScreenWidth; ++x) {
    for (size_t y = kScreenHeight; y < kPaddedScreenHeight; ++y) {
      buf[y * kPaddedScreenWidth + x] = kDarkPurple;
    }
  }
}

// Add this test function to display_impl.cpp
void TestPanelGeometry() {
  LV_LOG_INFO("Testing panel geometry with color blocks.");
  LV_LOG_INFO("Screen dimensions (%lu, %lu) - Padded to (%lu, %lu)", kScreenWidth, kScreenHeight, kPaddedScreenWidth,
              kPaddedScreenHeight);

  // Allocate test buffer (full frame)
  uint16_t* test_buf = (uint16_t*)heap_caps_malloc(kFramebufferSizeBytes, MALLOC_CAP_SPIRAM);
  if (!test_buf) {
    FLOG_ERROR("Failed to allocate test buffer");
    return;
  }

  // Fill with black background
  clearScreen(test_buf);

  // Test pattern 1: First 536 columns RED, remaining columns BLUE (if RAM is 536×240)
  // for (int y = 0; y < 180; y++) {
  //   for (int x = 0; x < 640; x++) {
  //     if (x < 536) {
  //       test_buf[y * 640 + x] = 0xF800;  // Red
  //     } else {
  //       test_buf[y * 640 + x] = 0x001F;  // Blue
  //     }
  //   }
  // }
  for (size_t y = 0; y < kScreenHeight; y++) {
    for (size_t x = 0; x < kScreenWidth; x++) {
      if (y < 536) {
        test_buf[y * kPaddedScreenWidth + x] = kRed;
      } else {
        test_buf[y * kPaddedScreenWidth + x] = kBlue;
      }
    }
  }

  LV_LOG_INFO("Sending test pattern: Red 0-535, Blue 536-639");
  amoled_set_window(0, 0, kPaddedScreenWidth - 1, kPaddedScreenHeight - 1);
  amoled_push_buffer(test_buf, kFramebufferSizePixels);

  vTaskDelay(3000 / portTICK_PERIOD_MS);  // Show for 3 seconds
  esp_task_wdt_reset();

  // // Test pattern 2: Horizontal stripes every 60 rows
  // for (int y = 0; y < 180; y++) {
  //   uint16_t color;
  //   if (y < 60)
  //     color = 0xF800;  // Red
  //   else if (y < 120)
  //     color = 0x07E0;  // Green
  //   else
  //     color = 0x001F;  // Blue

  //   for (int x = 0; x < 640; x++) {
  //     test_buf[y * 640 + x] = color;
  //   }
  // }
  // Test pattern 2: Horizontal stripes every 213 rows (to match portrait orientation)
  clearScreen(test_buf);

  for (size_t y = 0; y < kScreenHeight; y++) {  // ← 640 rows (height)
    uint16_t color;
    if (y < 213)
      color = kRed;  // Red
    else if (y < 426)
      color = kGreen;  // Green
    else
      color = kBlue;  // Blue

    for (size_t x = 0; x < kScreenWidth; x++) {      // ← 180 columns (width)
      test_buf[y * kPaddedScreenWidth + x] = color;  // ← Correct portrait indexing
    }
  }

  LV_LOG_INFO("Sending horizontal stripes pattern");
  amoled_set_window(0, 0, kPaddedScreenWidth - 1, kPaddedScreenHeight - 1);
  amoled_push_buffer(test_buf, kFramebufferSizePixels);

  vTaskDelay(3000 / portTICK_PERIOD_MS);
  esp_task_wdt_reset();

  // Test pattern 3: Small squares at corners to verify coordinate mapping
  // Clear to black
  // for (int i = 0; i < 180 * 640; i++) {
  //   test_buf[i] = 0x0000;
  // }

  // // Top-left: Red 50×50
  // for (int y = 0; y < 50; y++) {
  //   for (int x = 0; x < 50; x++) {
  //     test_buf[y * 180 + x] = kRed;
  //   }

  //   for (int x = 180 - 50; x < 180; x++) {
  //     test_buf[y * 180 + x] = kGreen;
  //   }
  // }

  // // Bottom-left: Blue 50×50
  // for (int y = 640 - 50; y < 640; y++) {
  //   for (int x = 0; x < 50; x++) {
  //     test_buf[y * 180 + x] = kBlue;
  //   }

  //   for (int x = 180 - 50; x < 180; x++) {
  //     test_buf[y * 180 + x] = kYellow;
  //   }
  // }

  // LV_LOG_INFO("Sending corner squares pattern");
  // amoled_set_window(0, 0, 179, 639);
  // amoled_push_buffer(test_buf, 180 * 640);

  vTaskDelay(3000 / portTICK_PERIOD_MS);
  esp_task_wdt_reset();

  // Test pattern 3.1: Small squares at corners to verify coordinate mapping
  clearScreen(test_buf);

  for (size_t yStart = kLvglDrawBufferLines / 4; yStart < kScreenHeight; yStart += kLvglDrawBufferLines) {
    for (size_t y = yStart; y < yStart + kLvglDrawBufferLines / 2; ++y) {
      for (size_t x = 0; x < kLvglDrawBufferLines / 2; x++) {
        test_buf[y * kPaddedScreenWidth + x] = kRed;
        test_buf[y * kPaddedScreenWidth + 180 - x] = kBlue;
      }
    }
  }

  LV_LOG_INFO("Sending corner squares pattern");
  amoled_set_window(0, 0, kPaddedScreenWidth - 1, kPaddedScreenHeight - 1);
  amoled_push_buffer(test_buf, kFramebufferSizePixels);
  // for (size_t y = 0; y < kPaddedScreenHeight; y += kLvglDrawBufferLines) {
  //   amoled_set_window(0, y, kPaddedScreenWidth - 1, y + kLvglDrawBufferLines - 1);
  //   amoled_push_buffer(test_buf + y * kPaddedScreenWidth, kPaddedScreenWidth * kLvglDrawBufferLines);
  // }

  // vTaskDelay(3000 / portTICK_PERIOD_MS);
  // esp_task_wdt_reset();

  // // Test 4: Fill buffer as 640×180 (landscape) instead of 180×640 (portrait)
  // for (int y = 0; y < 640; y++) {  // ← 640 rows
  //   uint16_t color;
  //   if (y < 213)
  //     color = 0x07E0;  // Green
  //   else if (y < 426)
  //     color = 0xF800;  // Red
  //   else
  //     color = 0x001F;  // Blue

  //   for (int x = 0; x < 180; x++) {  // ← 180 columns
  //     test_buf[y * 180 + x] = color;
  //   }
  // }

  // amoled_set_window(0, 0, 179, 639);
  // amoled_push_buffer(test_buf, 180 * 640);

  // heap_caps_free(test_buf);
  // LV_LOG_INFO("Test patterns complete");
}

void Backlight() {
  // Give lvgl time to render the first screen before turning on the backlight
  esp_timer_handle_t backlight_timer = NULL;
  const esp_timer_create_args_t timer_args = {
      .callback = BacklightTimerCallback, .arg = NULL, .name = "backlight_timer"};
  ESP_ERROR_CHECK(esp_timer_create(&timer_args, &backlight_timer));
  ESP_ERROR_CHECK(esp_timer_start_once(backlight_timer, 100000));
}

void BacklightTimerCallback(void* arg) {
  ESP_ERROR_CHECK(gpio_set_direction(kLcdBacklightPin, GPIO_MODE_OUTPUT));
  gpio_set_level(kLcdBacklightPin, 1);  // TODO: make configurable
};

}  // namespace impl
}  // namespace display