// cSpell: words lvgl qspi
#include "display_impl.hpp"

#include <driver/gpio.h>
#include <esp_check.h>
#include <esp_lcd_panel_ops.h>
#include <esp_lcd_panel_rgb.h>
#include <esp_lcd_panel_vendor.h>
#include <esp_lcd_touch_gt911.h>
#include <esp_timer.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

#include <map>

#include "funlog.h"
#include "i2c_manager.hpp"
#include "sdkconfig.h"

namespace display {
namespace impl {

spi_host_device_t _spi_host;
lv_display_t* _display;
uint64_t _lvgl_sleep;

i2c_master_dev_handle_t _dev_handle;
i2c_master_bus_handle_t _bus_handle;

esp_err_t DisplayPanelSetup() {
  ESP_ERROR_CHECK(gpio_set_direction(kLcdBacklightPin, GPIO_MODE_OUTPUT));
  gpio_set_level(kLcdBacklightPin, 1);
  // SetupQSPI();

  LV_LOG_USER("Install ILI9485 panel driver");
  esp_lcd_panel_handle_t panel_handle = NULL;
  LV_LOG_USER("Clock speed: %d MHz", (int)kLcdPixelClockHz / 1000000);

  esp_lcd_rgb_panel_config_t panel_config = {
      .clk_src = LCD_CLK_SRC_DEFAULT,
      // The timing parameters should refer to your LCD spec
      .timings =
          {
              .pclk_hz = kLcdPixelClockHz,
              .h_res = kLcdHRes,
              .v_res = kLcdVRes,
              .hsync_pulse_width = kLcdHSyncPulseWidth,
              .hsync_back_porch = kLcdHSyncBackPorch,
              .hsync_front_porch = kLcdHSyncFrontPorch,
              .vsync_pulse_width = kLcdVSyncPulseWidth,
              .vsync_back_porch = kLcdHSyncBackPorch,
              .vsync_front_porch = kLcdVSyncFrontPorch,
              .flags =
                  {
                      .hsync_idle_low = false,
                      .vsync_idle_low = false,
                      .de_idle_high = false,
                      .pclk_active_neg = true,
                      .pclk_idle_high = false  // end pclk_active_neg
                  }  // end flags
          },
      .data_width = 16,                        // RGB565 in parallel mode, thus 16 bits in width
      .bits_per_pixel = 0,                     //
      .num_fbs = 2,                            // allocate double frame buffer
      .bounce_buffer_size_px = kLcdHRes * 10,  // small buffer for when psram isn't available (like on nvs writes)
      .sram_trans_align = 0,                   //
      .psram_trans_align = 64,                 //
      .hsync_gpio_num = kLcdHSyncPin,
      .vsync_gpio_num = kLcdVSyncPin,
      .de_gpio_num = kLcdDePin,
      .pclk_gpio_num = kLcdPixelClockPin,
      .disp_gpio_num = NULL,
      .data_gpio_nums =
          {
              kLcdBlue0Pin, kLcdBlue1Pin, kLcdBlue2Pin, kLcdBlue3Pin,
              kLcdBlue4Pin,  //<! BLUE
              kLcdGreen0Pin, kLcdGreen1Pin, kLcdGreen2Pin, kLcdGreen3Pin, kLcdGreen4Pin,
              kLcdGreen5Pin,  //<! GREEN
              kLcdRed0Pin, kLcdRed1Pin, kLcdRed2Pin, kLcdRed3Pin,
              kLcdRed4Pin  //<! RED

          },

      .flags = {
          .disp_active_low = 0,
          .refresh_on_demand = 0,
          .fb_in_psram = true,
          .double_fb = true,
          .no_fb = 0,
          .bb_invalidate_cache = 0  //
      }};
  ESP_ERROR_CHECK(esp_lcd_new_rgb_panel(&panel_config, &panel_handle));  // io_handle, &panel_config, &panel_handle));
  ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_handle));
  ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle));
  // ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel_handle, true));

  // ESP_ERROR_CHECK(esp_lcd_panel_invert_color(panel_handle, true));

  LV_LOG_USER("Initialize LVGL");
  lv_init();

  _display = lv_display_create(kLcdHRes, kLcdVRes);
  if (!_display) {
    LV_LOG_ERROR("Failed to create LVGL display");
    return ESP_ERR_INVALID_STATE;
  }
  lv_display_set_dpi(_display, kLcdDPI);
  LV_LOG_USER("Display resolution: %dx%d, %d DPI", kLcdHRes, kLcdVRes, kLcdDPI);

  {
    LV_LOG_USER("Allocating LVGL buffers from PSRAM");
    void* buf1 = heap_caps_malloc(kDrawBufferSize, MALLOC_CAP_SPIRAM);
    assert(buf1);
    void* buf2 = heap_caps_malloc(kDrawBufferSize, MALLOC_CAP_SPIRAM);
    assert(buf2);

    LV_LOG_USER("Register buffers and display callback with LVGL");
    lv_display_set_buffers(_display, buf1, buf2, kDrawBufferSize, LV_DISPLAY_RENDER_MODE_PARTIAL);
    // lv_display_set_buffers(_display, buf1, buf2, kLcdHRes * kLcdVRes / 10, LV_DISPLAY_RENDER_MODE_FULL);
    lv_display_set_user_data(_display, panel_handle);
    lv_display_set_flush_cb(_display, LvglFlushCallback);
    LV_LOG_USER("buf1=%p buf2=%p expect_bytes=%u", buf1, buf2, (unsigned)(kDrawBufferSize));
  }

  // void* buf1 = NULL;
  // void* buf2 = NULL;
  // LV_LOG_USER("Use frame buffers as LVGL draw buffers");
  // ESP_ERROR_CHECK(esp_lcd_rgb_panel_get_frame_buffer(panel_handle, 2, &buf1, &buf2));

  // // lv_display_set_buffers(_display, buf1, buf2, kDrawBufferSize, LV_DISPLAY_RENDER_MODE_PARTIAL);
  // lv_display_set_buffers(_display, buf1, buf2, kLcdHRes * kLcdVRes * sizeof(lv_color_t),
  // LV_DISPLAY_RENDER_MODE_DIRECT);

  // // This project uses PARTIAL mode with CUSTOM ALLOCATED BUFFERS!
  // void* buf1 = heap_caps_malloc(kLcdHRes * kLcdVRes / 10, MALLOC_CAP_SPIRAM);
  // void* buf2 = heap_caps_malloc(kLcdHRes * kLcdVRes / 10, MALLOC_CAP_SPIRAM);
  // lv_display_set_buffers(_display, buf1, buf2, kLcdHRes * kLcdVRes / 10, LV_DISPLAY_RENDER_MODE_PARTIAL);

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
  // i2c_device_config_t i2c_dev_conf = {
  //     .dev_addr_length = I2C_ADDR_BIT_LEN_7,
  //     .device_address = kTouchI2cAddress,
  //     .scl_speed_hz = I2cManager::kClockSpeedHz,
  // };
  // esp_err_t err = I2cManager::GetInstance()->AddDevice(&i2c_dev_conf, &_dev_handle);
  // if (err != ESP_OK) {
  //   LV_LOG_ERROR("Failed to add touch device: %s", esp_err_to_name(err));
  //   return err;
  // }
  _bus_handle = I2cManager::GetInstance()->GetBusHandle();

  esp_lcd_panel_io_handle_t tp_io_handle = nullptr;
  esp_lcd_panel_io_i2c_config_t tp_io_config = ESP_LCD_TOUCH_IO_I2C_GT911_CONFIG();
  tp_io_config.scl_speed_hz = I2cManager::kClockSpeedHz;

  esp_lcd_touch_io_gt911_config_t tp_gt911_config = {
      .dev_addr = kTouchI2cAddress,
  };

  esp_lcd_touch_config_t tp_cfg = {
      .x_max = 800,  // kLcdHRes,
      .y_max = 480,  // kLcdVRes,
      .rst_gpio_num = kTouchI2cResetPin,
      .int_gpio_num = GPIO_NUM_NC,
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
      .process_coordinates = TouchMapCoordinates,
      .driver_data = &tp_gt911_config,
  };

  esp_lcd_new_panel_io_i2c_v2(_bus_handle, &tp_io_config, &tp_io_handle);

  esp_lcd_touch_handle_t tp;
  esp_lcd_touch_new_i2c_gt911(tp_io_handle, &tp_cfg, &tp);

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
  width = kLcdHRes;
  height = kLcdVRes;
  // }
}

lv_display_t* GetDisplayObjPtr() { return _display; }

esp_err_t SetupQSPI() {
  // LV_LOG_USER("Initialize QSPI bus");
  // const spi_bus_config_t buscfg = {
  //     .data0_io_num = CONFIG_IMPL_LILYGO_TDISPLAY_S3_LONG_QSPI_D0_PIN,
  //     .data1_io_num = CONFIG_IMPL_LILYGO_TDISPLAY_S3_LONG_QSPI_D1_PIN,
  //     .sclk_io_num = CONFIG_IMPL_LILYGO_TDISPLAY_S3_LONG_QSPI_SCK_PIN,
  //     .data2_io_num = CONFIG_IMPL_LILYGO_TDISPLAY_S3_LONG_QSPI_D2_PIN,
  //     .data3_io_num = CONFIG_IMPL_LILYGO_TDISPLAY_S3_LONG_QSPI_D3_PIN,
  //     .max_transfer_sz = kDrawBufferSize,
  // };
  // ESP_ERROR_CHECK(spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_CH_AUTO));

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

  lv_display_flush_ready(disp);

  LV_LOG_TRACE("LVGL flush time: %lli us", (esp_timer_get_time() - starter));
}

uint16_t map(uint16_t n, uint16_t in_min, uint16_t in_max, uint16_t out_min, uint16_t out_max) {
  return (n - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

void TouchMapCoordinates(esp_lcd_touch_handle_t tp, uint16_t* x, uint16_t* y, uint16_t* strength, uint8_t* point_num,
                         uint8_t max_point_num) {
  *x = map(*x, TOUCH_H_RES_MIN, TOUCH_H_RES_MAX, 0, 800);
  *y = map(*y, TOUCH_V_RES_MIN, TOUCH_V_RES_MAX, 0, 480);
};

// static bool example_notify_lvgl_flush_ready(esp_lcd_panel_handle_t panel,
// const esp_lcd_rgb_panel_event_data_t* event_data, void* user_ctx) {};
bool LvglFlushReadyCallback(esp_lcd_panel_handle_t panel, const esp_lcd_rgb_panel_event_data_t* edata, void* user_ctx) {
  lv_display_t* disp = (lv_display_t*)user_ctx;
  LV_LOG_TRACE("LVGL flush ready callback triggered");
  lv_display_flush_ready(disp);
  return false;
};

void LvglTouchCallback(lv_indev_t* indev, lv_indev_data_t* data) {
  uint16_t touchpad_x[1] = {0};
  uint16_t touchpad_y[1] = {0};
  uint8_t touchpad_cnt = 0;

  // // Get the touch controller handle from LVGL input device user data
  esp_lcd_touch_handle_t touch_handle = (esp_lcd_touch_handle_t)lv_indev_get_user_data(indev);

  // // Read current touch data from the controller
  esp_err_t ret = esp_lcd_touch_read_data(touch_handle);
  if (ret != ESP_OK) {
    // If read failed, report no touch
    data->state = LV_INDEV_STATE_RELEASED;
    return;
  }

  // // Get coordinates and touch state
  bool touchpad_pressed = esp_lcd_touch_get_coordinates(touch_handle, touchpad_x, touchpad_y, NULL, &touchpad_cnt, 1);

  if (touchpad_pressed && touchpad_cnt > 0) {
    data->point.x = touchpad_x[0];
    data->point.y = touchpad_y[0];
    data->state = LV_INDEV_STATE_PRESSED;
    LV_LOG_TRACE("Touch data: x=%d y=%d state=%d", data->point.x, data->point.y, data->state);

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
  // Backlight();
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
  gpio_set_level(kLcdBacklightPin, 1);
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