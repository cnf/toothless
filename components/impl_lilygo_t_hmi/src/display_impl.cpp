// cSpell: words lvgl qspi
#include "display_impl.hpp"

#include <driver/gpio.h>
#include <esp_check.h>
#include <esp_lcd_panel_ops.h>
#include <esp_lcd_panel_rgb.h>
#include <esp_lcd_panel_vendor.h>
#include <esp_lcd_touch_xpt2046.h>
#include <esp_timer.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

#include "sdkconfig.h"

namespace display {
namespace impl {

spi_host_device_t _spi_host;
lv_display_t* _display;
uint64_t _lvgl_sleep;

esp_err_t DisplayPanelSetup() {
  // ESP_ERROR_CHECK(gpio_set_direction(kTftBacklightPin, GPIO_MODE_OUTPUT));
  // gpio_set_level(kTftBacklightPin, 1);

  ESP_ERROR_CHECK(gpio_set_direction(kPowerEnablePin, GPIO_MODE_OUTPUT));
  gpio_set_level(kPowerEnablePin, 1);

  vTaskDelay(pdMS_TO_TICKS(500));

  // SetupQSPI();

  esp_lcd_i80_bus_handle_t i80_bus = NULL;
  esp_lcd_i80_bus_config_t bus_config = {.dc_gpio_num = kTftDCPin,
                                         .wr_gpio_num = kTftWRPin,
                                         .clk_src = LCD_CLK_SRC_DEFAULT,
                                         .data_gpio_nums =
                                             {
                                                 kTftData0Pin,
                                                 kTftData1Pin,
                                                 kTftData2Pin,
                                                 kTftData3Pin,
                                                 kTftData4Pin,
                                                 kTftData5Pin,
                                                 kTftData6Pin,
                                                 kTftData7Pin,
                                             },
                                         .bus_width = 8,
                                         .max_transfer_bytes = kDisplayBufferBytes,  // kHRes * 100 * sizeof(uint16_t),
                                         .psram_trans_align = 64,
                                         .sram_trans_align = 4};
  esp_lcd_new_i80_bus(&bus_config, &i80_bus);

  static esp_lcd_panel_io_handle_t io_handle = NULL;
  esp_lcd_panel_io_i80_config_t io_config = {
      .cs_gpio_num = kTftCSPin,
      .pclk_hz = kPixelClockHz,
      .trans_queue_depth = 10,
      // .on_color_trans_done = LvglFlushReadyCallback,
      // .user_ctx = &disp_drv,
      .lcd_cmd_bits = kTftCmdBits,
      .lcd_param_bits = kTftParamBits,
      .dc_levels =
          {
              .dc_idle_level = 0,
              .dc_cmd_level = 0,
              .dc_dummy_level = 0,
              .dc_data_level = 1,
          },
      .flags =
          {
              .swap_color_bytes = 1,
          },
  };
  ESP_ERROR_CHECK(esp_lcd_new_panel_io_i80(i80_bus, &io_config, &io_handle));

  static esp_lcd_panel_handle_t panel_handle = NULL;
  esp_lcd_panel_dev_config_t panel_config = {.reset_gpio_num = kTftResetPin,
                                             .color_space = ESP_LCD_COLOR_SPACE_RGB,
                                             .bits_per_pixel = 16,
                                             .vendor_config = NULL};
  ESP_ERROR_CHECK(esp_lcd_new_panel_st7789(io_handle, &panel_config, &panel_handle));

  // ESP_ERROR_CHECK(esp_lcd_new_panel(&panel_config, &panel_handle));  // io_handle, &panel_config, &panel_handle));
  ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_handle));
  ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle));

  esp_lcd_panel_mirror(panel_handle, false, true);

  esp_lcd_panel_swap_xy(panel_handle, true);

  esp_lcd_panel_disp_on_off(panel_handle, true);

  // ESP_ERROR_CHECK(esp_lcd_panel_invert_color(panel_handle, true));

  LV_LOG_USER("Initialize LVGL");
  lv_init();

  _display = lv_display_create(kHRes, kVRes);
  if (!_display) {
    LV_LOG_ERROR("Failed to create LVGL display");
    return ESP_ERR_INVALID_STATE;
  }
  lv_display_set_dpi(_display, kLcdDPI);
  LV_LOG_USER("Display resolution: %dx%d", kHRes, kVRes);
  LV_LOG_USER("LV_COLOR_DEPTH=%d bytesPerPixel=%u sizeof(lv_color_t)=%u buffer_bytes=%u", LV_COLOR_DEPTH,
              (unsigned)kBytesPerPixel, (unsigned)sizeof(lv_color_t), (unsigned)kDisplayBufferBytes);
  lv_display_set_color_format(_display, LV_COLOR_FORMAT_RGB565);

  {
    LV_LOG_USER("Allocating LVGL buffers from PSRAM");
    lv_color_t* buf1 = (lv_color_t*)heap_caps_malloc(kDisplayBufferBytes, MALLOC_CAP_SPIRAM);
    // assert(buf1);
    if (!buf1) {
      LV_LOG_ERROR("DMA buf1 alloc failed (bytes=%u free_dma=%u free_psram=%u)", (unsigned)kDisplayBufferBytes,
                   (unsigned)heap_caps_get_free_size(MALLOC_CAP_DMA),
                   (unsigned)heap_caps_get_free_size(MALLOC_CAP_SPIRAM));
      return ESP_ERR_NO_MEM;
    }
    lv_color_t* buf2 = (lv_color_t*)heap_caps_malloc(kDisplayBufferBytes, MALLOC_CAP_SPIRAM);
    // assert(buf2);
    if (!buf2) {
      free(buf1);
      LV_LOG_ERROR("DMA buf2 alloc failed");
      return ESP_ERR_NO_MEM;
    }

    LV_LOG_USER("Register buffers and display callback with LVGL");
    lv_display_set_buffers(_display, buf1, buf2, kDisplayBufferBytes, LV_DISPLAY_RENDER_MODE_PARTIAL);
    lv_display_set_user_data(_display, panel_handle);
    lv_display_set_flush_cb(_display, LvglFlushCallback);
    LV_LOG_USER("buf1=%p buf2=%p expect_bytes=%u", buf1, buf2, (unsigned)(kDisplayBufferBytes));
  }

  // {
  //   LV_LOG_USER("Register io panel event callback for LVGL flush ready notification");
  //   esp_lcd_rgb_panel_event_callbacks_t cbs = {
  //       .on_color_trans_done = LvglFlushReadyCallback,
  //       .on_vsync = nullptr,
  //   };

  //   /* Register done callback */
  //   ESP_ERROR_CHECK(esp_lcd_rgb_panel_register_event_callbacks(panel_handle, &cbs, _display));
  // }

  return ESP_OK;
}  // namespace impl

esp_err_t TouchPanelSetup() {
  SetupSPI();
  LV_LOG_USER("Initialize XPT4026 touch controller");

  esp_lcd_touch_handle_t tp = NULL;
  esp_lcd_panel_io_handle_t tp_io_handle = NULL;
  esp_lcd_panel_io_spi_config_t tp_io_config = ESP_LCD_TOUCH_IO_SPI_XPT2046_CONFIG(kTouchCSPin);
  ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)SPI2_HOST, &tp_io_config, &tp_io_handle));

  esp_lcd_touch_config_t tp_cfg = {
      .x_max = kVRes,
      .y_max = kHRes,
      .rst_gpio_num = GPIO_NUM_NC,
      .int_gpio_num = kTouchIntPin,
      .flags =
          {
              .swap_xy = 1,
              .mirror_x = 0,
              .mirror_y = 1,
          },
  };

  LV_LOG_USER("Initialize touch controller XPT2046");
  ESP_ERROR_CHECK(esp_lcd_touch_new_spi_xpt2046(tp_io_handle, &tp_cfg, &tp));

  static lv_indev_t* indev;
  indev = lv_indev_create();  // Input device driver (SetupTouchPanel)
  lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
  lv_indev_set_display(indev, _display);
  lv_indev_set_user_data(indev, tp);

  lv_indev_set_read_cb(indev, LvglTouchCallback);
  return ESP_OK;
}

void GetDisplayDimensions(uint16_t& width, uint16_t& height) {
  width = kHRes;   // CONFIG_IMPL_LILYGO_TDISPLAY_S3_LONG_HRES;
  height = kVRes;  // CONFIG_IMPL_LILYGO_TDISPLAY_S3_LONG_VRES;
}

lv_display_t* GetDisplayObjPtr() { return _display; }

esp_err_t SetupSPI() {
  LV_LOG_USER("Initialize SPI bus");
  spi_bus_config_t buscfg = {
      .mosi_io_num = kTouchMosiPin,
      .miso_io_num = kTouchMisoPin,
      .sclk_io_num = kTouchSclkPin,
      .quadwp_io_num = -1,
      .quadhd_io_num = -1,
      // .max_transfer_sz = kMaxTransferSize,
  };
  esp_err_t r = spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_CH_AUTO);
  if (r != ESP_OK) {
    LV_LOG_ERROR("spi_bus_initialize failed: %d", r);
    return r;
  }
  return ESP_OK;
}

void LvglFlushCallback(lv_display_t* disp, const lv_area_t* area, uint8_t* px_map) {
  [[maybe_unused]] uint32_t starter = esp_timer_get_time();
  LV_LOG_TRACE("LVGL Flush Callback: area x1=%li y1=%li x2=%li y2=%li", area->x1, area->y1, area->x2, area->y2);

  esp_lcd_panel_handle_t panel_handle = (esp_lcd_panel_handle_t)lv_display_get_user_data(disp);

  int offsetx1 = area->x1;
  int offsetx2 = area->x2;
  int offsety1 = area->y1;
  int offsety2 = area->y2;

  esp_lcd_panel_draw_bitmap(panel_handle, offsetx1, offsety1, offsetx2 + 1, offsety2 + 1, px_map);

  // lv_display_flush_ready HAS to be there, or we lock, and trigger WDT
  lv_display_flush_ready(disp);
  LV_LOG_TRACE("LVGL flush time: %lli us", (esp_timer_get_time() - starter));
}

// static bool example_notify_lvgl_flush_ready(esp_lcd_panel_handle_t panel,
// const esp_lcd_rgb_panel_event_data_t* event_data, void* user_ctx) {};
bool LvglFlushReadyCallback(esp_lcd_panel_io_handle_t panel, esp_lcd_panel_io_event_data_t* edata, void* user_ctx) {
  lv_display_t* disp = (lv_display_t*)user_ctx;
  LV_LOG_TRACE("LVGL flush ready callback triggered");
  // lv_display_flush_ready(disp);
  lv_display_flush_ready(_display);
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
    LV_LOG_TRACE("Touch data: state=%s x=%u y=%u", data->state == LV_INDEV_STATE_PRESSED ? "PRESSED" : "RELEASED",
                 (unsigned)data->point.x, (unsigned)data->point.y);
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
  // lv_obj_t* boot_scr = lv_obj_create(NULL);
  // lv_obj_set_style_bg_color(boot_scr, lv_color_make(0, 100, 200), 0);
  // lv_obj_set_style_bg_opa(boot_scr, LV_OPA_COVER, 0);
  // lv_screen_load(boot_scr);
  // Backlight();

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

  // lv_display_flush_ready(_display);  // trigger LVGL flush

  lv_screen_load(boot_scr);
  Backlight();
}

void Backlight() {
  // Give lvgl time to render the first screen before turning on the backlight
  esp_timer_handle_t backlight_timer = NULL;
  const esp_timer_create_args_t timer_args = {
      .callback = BacklightTimerCallback, .arg = NULL, .name = "backlight_timer"};
  ESP_ERROR_CHECK(esp_timer_create(&timer_args, &backlight_timer));
  ESP_ERROR_CHECK(esp_timer_start_once(backlight_timer, 100 * 1000));
}

void BacklightTimerCallback(void* arg) {
  LV_LOG_USER("Turning on TFT backlight");
  ESP_ERROR_CHECK(gpio_set_direction(kTftBacklightPin, GPIO_MODE_OUTPUT));
  gpio_set_level(kTftBacklightPin, 1);
};

[[maybe_unused]] IRAM_ATTR static void DrawBitmap(esp_lcd_panel_handle_t panel_handle, uint32_t h_res, uint32_t v_res) {
  //   refresh_finish = xSemaphoreCreateBinary();
  //   assert(refresh_finish);
  // #define TEST_LCD_BIT_PER_PIXEL 16
  //   uint16_t row_line = ((v_res / TEST_LCD_BIT_PER_PIXEL) << 1) >> 1;
  //   uint8_t byte_per_pixel = TEST_LCD_BIT_PER_PIXEL / 8;
  //   uint8_t* color = (uint8_t*)heap_caps_calloc(1, row_line * h_res * byte_per_pixel, MALLOC_CAP_DMA);
  //   assert(color);

  //   for (int j = 0; j < TEST_LCD_BIT_PER_PIXEL; j++) {
  //     for (int i = 0; i < row_line * h_res; i++) {
  //       for (int k = 0; k < byte_per_pixel; k++) {
  //         color[i * byte_per_pixel + k] = (SPI_SWAP_DATA_TX(BIT(j), TEST_LCD_BIT_PER_PIXEL) >> (k * 8)) & 0xff;
  //       }
  //     }
  //     ESP_ERROR_CHECK(esp_lcd_panel_draw_bitmap(panel_handle, 0, j * row_line, h_res, (j + 1) * row_line, color));
  //     xSemaphoreTake(refresh_finish, portMAX_DELAY);
  //   }
  //   free(color);
  //   vSemaphoreDelete(refresh_finish);
}

}  // namespace impl
}  // namespace display