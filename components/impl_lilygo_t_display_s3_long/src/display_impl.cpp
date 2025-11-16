// cSpell: words lvgl qspi
#include "display_impl.hpp"

#include <driver/gpio.h>
#include <driver/spi_master.h>
#include <esp_check.h>
#include <esp_lcd_axs15231b.h>
#include <esp_lcd_panel_ops.h>
#include <esp_lcd_panel_vendor.h>
#include <esp_timer.h>

#include "sdkconfig.h"

namespace display {
namespace impl {

// spi_host_device_t _spi_host;
lv_display_t* _display;
uint64_t _lvgl_sleep;
// static spi_device_handle_t _spi = NULL;

static SemaphoreHandle_t refresh_finish = NULL;

// Simple test: fill screen with red
void TestDisplayRaw(esp_lcd_panel_handle_t& panel) {
  uint16_t* test_buf = (uint16_t*)malloc(640 * 10 * sizeof(uint16_t));
  if (!test_buf) return;

  // Fill with red (RGB565: 0xF800)
  for (int i = 0; i < 640 * 10; i++) {
    test_buf[i] = 0xF800;
  }

  // Draw the buffer to different areas of screen
  // esp_lcd_panel_handle_t panel = ...;  // You'll need to pass this
  for (int y = 0; y < 180; y += 10) {
    esp_lcd_panel_draw_bitmap(panel, 0, y, 640, y + 10, test_buf);
  }

  free(test_buf);
}

esp_err_t DisplayPanelSetup() {
  ESP_ERROR_CHECK(gpio_set_direction(kLcdBacklightPin, GPIO_MODE_OUTPUT));
  gpio_set_level(kLcdBacklightPin, 1);  // BUG: Remove here

  ESP_RETURN_ON_ERROR(SetupQSPI(), "impl: LilyGo T-Display S3 Long", "QSPI Setup failed");
  LV_LOG_USER("Attach the LCD IO to the SPI bus");
  esp_lcd_panel_io_handle_t io_handle = NULL;
  esp_lcd_panel_io_spi_config_t io_config = {
      .cs_gpio_num = kLcdCsPin,
      .dc_gpio_num = -1,
      .spi_mode = 3,
      .pclk_hz = kLcdPixelClockHz,
      .trans_queue_depth = 10,
      .lcd_cmd_bits = kLcdCmdBits,
      .lcd_param_bits = kLcdParamBits,
      .flags =
          {
              .quad_mode = true,
          },
  };
  ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi(SPI3_HOST, &io_config, &io_handle));

  // static const axs15231b_lcd_init_cmd_t lcd_init_cmds[] = {
  //     {0x28, NULL, 0, 0x40},  // DISPOFF + 64ms delay
  //     {0x10, NULL, 0, 0x20},  // SLPIN + 32ms delay
  //     {0x11, NULL, 0, 0x80},  // SLPOUT + 128ms delay
  //     {0x29, NULL, 0, 0x00},  // DISPON + no delay
  // };
  static const axs15231b_lcd_init_cmd_t vendor_init[] = {
      {0x28, (uint8_t[]){0x00}, 1, 20},   // DISPOFF, 20ms delay
      {0x10, (uint8_t[]){0x00}, 1, 20},   // SLPIN, 20ms delay
      {0x11, (uint8_t[]){0x00}, 1, 200},  // SLPOUT, 200ms delay
      {0x29, (uint8_t[]){0x00}, 1, 0},    // DISPON, no delay
  };
  LV_LOG_USER("Install AXS15231B panel driver");
  esp_lcd_panel_handle_t panel_handle = NULL;
  axs15231b_vendor_config_t vendor_config = {
      .init_cmds = vendor_init,  // Uncomment these line if use custom initialization commands
      .init_cmds_size = sizeof(vendor_init) / sizeof(axs15231b_lcd_init_cmd_t),
      // .init_cmds = NULL,
      // .init_cmds_size = 0,
      .flags =
          {
              .use_qspi_interface = 1,
          },
  };
  esp_lcd_panel_dev_config_t panel_config = {
      .reset_gpio_num = kLcdResetPin,
      .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB,  // Implemented by LCD command `36h`
      .bits_per_pixel = 16,                        // Implemented by LCD command `3Ah` (16/18)
      .vendor_config = &vendor_config,
  };
  ESP_ERROR_CHECK(esp_lcd_new_panel_axs15231b(io_handle, &panel_config, &panel_handle));
  ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_handle));
  LV_LOG_USER("Initialize the LCD panel");
  vTaskDelay(pdMS_TO_TICKS(25));
  LV_LOG_USER("Calling esp_lcd_panel_init()");
  ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle));
  LV_LOG_USER("LCD panel initialized");
  vTaskDelay(pdMS_TO_TICKS(25));
  TestDisplayRaw(panel_handle);
  LV_LOG_USER("Delay 2.5s after test pattern");
  vTaskDelay(pdMS_TO_TICKS(2500000));

  ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel_handle, true));

  // ESP_ERROR_CHECK(esp_lcd_panel_invert_color(panel_handle, true));

  LV_LOG_USER("Initialize LVGL");
  lv_init();

  _display = lv_display_create(kHres, kVres);
  if (!_display) {
    LV_LOG_ERROR("Failed to create LVGL display");
    return ESP_ERR_INVALID_STATE;
  }

  LV_LOG_USER("Display resolution: %lix%li", kHres, kVres);

  LV_LOG_USER("Draw buffer size: %zu bytes (%d lines)", kDrawBufferSize, kLvglDrawBufferLines);

  // void* buf1 = heap_caps_malloc(kDrawBufferSize, MALLOC_CAP_SPIRAM | MALLOC_CAP_INTERNAL);
  void* buf1 = spi_bus_dma_memory_alloc(SPI3_HOST, kDrawBufferSize, 0);
  if (!buf1) {
    LV_LOG_ERROR("DMA buf1 alloc failed");
    return ESP_ERR_NO_MEM;
  }

  // void* buf2 = heap_caps_malloc(kDrawBufferSize, MALLOC_CAP_SPIRAM | MALLOC_CAP_INTERNAL);
  void* buf2 = spi_bus_dma_memory_alloc(SPI3_HOST, kDrawBufferSize, 0);
  if (!buf2) {
    free(buf1);
    LV_LOG_ERROR("DMA buf2 alloc failed");
    return ESP_ERR_NO_MEM;
  }

  // Clear buffers to avoid garbage pixels
  // memset(buf1, 0x00, kDrawBufferSize);
  // memset(buf2, 0x00, kDrawBufferSize);

  LV_LOG_USER("Allocated DMA buffers: buf1=%p, buf2=%p", buf1, buf2);
  // initialize LVGL draw buffers
  lv_display_set_buffers(_display, buf1, buf2, kDrawBufferSize, LV_DISPLAY_RENDER_MODE_PARTIAL);

  // associate the mipi panel handle to the display
  lv_display_set_user_data(_display, panel_handle);
  // set color depth
  lv_display_set_color_format(_display, LV_COLOR_FORMAT_RGB565);

  // ESP_ERROR_CHECK(esp_lcd_panel_swap_xy(panel_handle, true));
  // ESP_ERROR_CHECK(esp_lcd_panel_mirror(panel_handle, true, true));

  // set the callback which can copy the rendered image to an area of the display
  lv_display_set_flush_cb(_display, LvglFlushCallback);
  {
    LV_LOG_USER("Register io panel event callback for LVGL flush ready notification");
    const esp_lcd_panel_io_callbacks_t cbs = {
        .on_color_trans_done = LvglFlushReadyCallback,
    };

    /* Register done callback */
    ESP_ERROR_CHECK(esp_lcd_panel_io_register_event_callbacks(io_handle, &cbs, _display));
    // lv_display_add_event_cb(_display, lvgl_display_event_cb, LV_EVENT_REFR_READY, NULL);
  }

  return ESP_OK;
}

esp_err_t TouchPanelSetup() {
  // bool swapxy, mirror_x, mirror_y;
  // uint16_t hres, vres;
  // if (PORTRAIT) {
  //   swapxy = false;
  //   mirror_x = 0;
  //   mirror_y = 0;
  //   hres = ;
  //   vres = ;
  // } else {
  // swapxy = true;
  // mirror_x = 0;
  // mirror_y = 1;
  // hres = ;
  // vres = ;
  // }
  LV_LOG_USER("Initialize AXS15231B touch controller");

  esp_lcd_panel_io_handle_t tp_io_handle = nullptr;
  esp_lcd_panel_io_i2c_config_t tp_io_config = ESP_LCD_TOUCH_IO_I2C_AXS15231B_CONFIG();

  esp_lcd_touch_config_t tp_cfg = {
      .x_max = kHres,
      .y_max = kVres,
      // .rst_gpio_num = -1,
      // .int_gpio_num = -1,
      .levels =
          {
              .reset = 0,
              .interrupt = 0,
          },
      .flags =
          {
              .swap_xy = 0,
              .mirror_x = 0,
              .mirror_y = 0,
          },
      .driver_data = &tp_io_config,
  };

  esp_lcd_touch_handle_t tp;
  esp_lcd_touch_new_i2c_axs15231b(tp_io_handle, &tp_cfg, &tp);

  static lv_indev_t* indev;
  indev = lv_indev_create();  // Input device driver (SetupTouchPanel)
  lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
  lv_indev_set_display(indev, _display);
  lv_indev_set_user_data(indev, tp);

  lv_indev_set_read_cb(indev, LvglTouchCallback);
  return ESP_OK;
}

void GetDisplayDimensions(uint16_t& width, uint16_t& height) {
  // if (PORTRAIT) {
  //   width = IMPL_LILYGO_TDISPLAY_S3_LONG_VRES;
  //   height = IMPL_LILYGO_TDISPLAY_S3_LONG_HRES;
  // } else {
  width = kHres;
  height = kVres;
  // }
}

lv_display_t* GetDisplayObjPtr() { return _display; }

esp_err_t SetupQSPI() {
  static bool initialized = false;
  if (initialized) return ESP_OK;
  LV_LOG_USER("Initialize QSPI bus");
  const spi_bus_config_t buscfg = {
      .data0_io_num = kLcdData0Pin,
      .data1_io_num = kLcdData1Pin,
      .sclk_io_num = kLcdSckPin,
      .data2_io_num = kLcdData2Pin,
      .data3_io_num = kLcdData3Pin,
      .max_transfer_sz = kMaxTransferSize,  //(SEND_BUF_SIZE * 16) + 8,
      .flags = SPICOMMON_BUSFLAG_MASTER | SPICOMMON_BUSFLAG_GPIO_PINS,
  };
  LV_LOG_USER("Assigned DMA memory size: %d bytes [%d]", heap_caps_get_free_size(MALLOC_CAP_DMA), kMaxTransferSize);
  ESP_ERROR_CHECK(spi_bus_initialize(SPI3_HOST, &buscfg, SPI_DMA_CH_AUTO));
  // spi_device_interface_config_t devcfg = {
  //     .command_bits = 8,
  //     .address_bits = 24,
  //     .mode = 0,
  //     .clock_speed_hz = kLcdPixelClockHz,
  //     .spics_io_num = -1,
  //     .flags = SPI_DEVICE_HALFDUPLEX,
  //     .queue_size = 17,
  //     // .post_cb = spi_dma_cd,
  // };
  // ESP_ERROR_CHECK(spi_bus_add_device(SPI3_HOST, &devcfg, &_spi));
  // check if spi bus is actually initialized
  // TODO: need better check
  // while (true) {
  //   size_t free_size = heap_caps_get_free_size(MALLOC_CAP_DMA);
  //   if (free_size < kMaxTransferSize + (100 * 1024)) {  // Need max_transfer + 100KB buffer
  //     LV_LOG_WARN("SPI bus DMA memory not ready yet, free DMA heap: %u", free_size);
  //     vTaskDelay(pdMS_TO_TICKS(100));
  //   } else {
  //     break;
  //   }
  // };
  initialized = true;
  LV_LOG_USER("SPI initialized ok");

  return ESP_OK;
};

void LvglFlushCallback(lv_display_t* disp, const lv_area_t* area, uint8_t* px_map) {
  // LV_LOG_USER("FLUSH CALLED: area(%li,%li)-(%li,%li)", area->x1, area->y1, area->x2, area->y2);

  [[maybe_unused]] uint32_t starter = esp_timer_get_time();

  esp_lcd_panel_handle_t panel_handle = (esp_lcd_panel_handle_t)lv_display_get_user_data(disp);
  int offsetx1 = area->x1;
  int offsetx2 = area->x2;
  int offsety1 = area->y1;
  int offsety2 = area->y2;

  // because SPI LCD is big-endian, we need to swap the RGB bytes order
  // lv_draw_sw_rgb565_swap(px_map, (offsetx2 + 1 - offsetx1) * (offsety2 + 1 - offsety1));

  // copy a buffer's content to a specific area of the display
  // esp_lcd_panel_draw_bitmap(panel_handle, offsetx1, offsety1, offsetx2 + 1, offsety2 + 1, px_map);
  esp_err_t ret = esp_lcd_panel_draw_bitmap(panel_handle, offsetx1, offsety1, offsetx2 + 1, offsety2 + 1, px_map);
  if (ret != ESP_OK) {
    LV_LOG_ERROR("draw_bitmap failed: %s", esp_err_to_name(ret));
  } else {
    // LV_LOG_USER("Flush OK: area(%d,%d)-(%d,%d)", offsetx1, offsety1, offsetx2, offsety2);
  }

  // CRITICAL: Tell LVGL that flushing is done
  // lv_display_flush_ready(disp);
  LV_LOG_TRACE("LVGL flush time: %lli us", (esp_timer_get_time() - starter));
}

bool LvglFlushReadyCallback(esp_lcd_panel_io_handle_t io, esp_lcd_panel_io_event_data_t* edata, void* user_data) {
  lv_display_t* disp = (lv_display_t*)user_data;
  lv_display_flush_ready(disp);
  return false;
};

void LvglTouchCallback(lv_indev_t* indev, lv_indev_data_t* data) {
  uint16_t touchpad_x[1] = {0};
  uint16_t touchpad_y[1] = {0};
  uint8_t touchpad_cnt = 0;

  // Get the touch controller handle from LVGL input device user data
  esp_lcd_touch_handle_t touch_handle = (esp_lcd_touch_handle_t)lv_indev_get_user_data(indev);

  // Read current touch data from the controller
  esp_err_t ret = esp_lcd_touch_read_data(touch_handle);
  if (ret != ESP_OK) {
    // If read failed, report no touch
    data->state = LV_INDEV_STATE_RELEASED;
    return;
  }

  // Get coordinates and touch state
  bool touchpad_pressed = esp_lcd_touch_get_coordinates(touch_handle, touchpad_x, touchpad_y, NULL, &touchpad_cnt, 1);

  if (touchpad_pressed && touchpad_cnt > 0) {
    data->point.x = touchpad_x[0];
    data->point.y = touchpad_y[0];
    data->state = LV_INDEV_STATE_PRESSED;
  } else {
    // No touch detected
    data->state = LV_INDEV_STATE_RELEASED;
  }
  return;
}

void TurnOn() {
  lv_async_call([](void*) { ShowBootScreen(); }, nullptr);
};

void ShowBootScreen() {
  LV_LOG_USER("Displaying boot screen");
  // lv_lock();

  lv_obj_t* boot_scr = lv_obj_create(NULL);
  lv_obj_set_style_bg_color(boot_scr, lv_color_hex(0xFF0000), 0);
  lv_obj_set_style_bg_opa(boot_scr, LV_OPA_COVER, 0);

  // Simple test: create a red rectangle
  lv_obj_t* test_rect = lv_obj_create(boot_scr);
  lv_obj_set_size(test_rect, 100, 100);
  lv_obj_set_style_bg_color(test_rect, lv_color_hex(0x00FF00), 0);
  lv_obj_center(test_rect);

  lv_screen_load(boot_scr);
  return;
  /*
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

    lv_display_flush_ready(_display);  // trigger LVGL flush

    lv_screen_load(boot_scr);
    // lv_unlock();
    Backlight();
    */
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

[[maybe_unused]] IRAM_ATTR static void DrawBitmap(esp_lcd_panel_handle_t panel_handle, uint32_t h_res, uint32_t v_res) {
  refresh_finish = xSemaphoreCreateBinary();
  assert(refresh_finish);
#define TEST_LCD_BIT_PER_PIXEL 16
  uint16_t row_line = ((v_res / TEST_LCD_BIT_PER_PIXEL) << 1) >> 1;
  uint8_t byte_per_pixel = TEST_LCD_BIT_PER_PIXEL / 8;
  uint8_t* color = (uint8_t*)heap_caps_calloc(1, row_line * h_res * byte_per_pixel, MALLOC_CAP_DMA);
  assert(color);

  for (int j = 0; j < TEST_LCD_BIT_PER_PIXEL; j++) {
    for (int i = 0; i < row_line * h_res; i++) {
      for (int k = 0; k < byte_per_pixel; k++) {
        color[i * byte_per_pixel + k] = (SPI_SWAP_DATA_TX(BIT(j), TEST_LCD_BIT_PER_PIXEL) >> (k * 8)) & 0xff;
      }
    }
    ESP_ERROR_CHECK(esp_lcd_panel_draw_bitmap(panel_handle, 0, j * row_line, h_res, (j + 1) * row_line, color));
    xSemaphoreTake(refresh_finish, portMAX_DELAY);
  }
  free(color);
  vSemaphoreDelete(refresh_finish);
}

}  // namespace impl
}  // namespace display