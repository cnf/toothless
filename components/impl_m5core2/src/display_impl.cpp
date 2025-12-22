// cSpell: words lvgl qspi
#include "display_impl.hpp"

#include <button_gpio.h>
#include <driver/gpio.h>
#include <esp_check.h>
#include <esp_lcd_gc9a01.h>
#include <esp_lcd_panel_ops.h>
#include <esp_lcd_panel_rgb.h>
#include <esp_lcd_panel_vendor.h>
#include <esp_lcd_touch_ft5x06.h>
#include <esp_timer.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <iot_button.h>
#include <iot_knob.h>

#include "i2c_manager.hpp"
#include "sdkconfig.h"

namespace display {
namespace impl {

spi_host_device_t _spi_host;
lv_display_t* _display;
uint64_t _lvgl_sleep;

static volatile int32_t _s_enc_delta = 0;

i2c_master_dev_handle_t _dev_handle;
i2c_master_bus_handle_t _bus_handle;

lv_group_t* _default_group;

esp_err_t DisplayPanelSetup() {
  // ESP_ERROR_CHECK(gpio_set_direction(kTftBacklightPin, GPIO_MODE_OUTPUT));
  // gpio_set_level(kTftBacklightPin, 1);

  // ESP_ERROR_CHECK(gpio_set_direction(kPowerEnablePin, GPIO_MODE_OUTPUT));
  // gpio_set_level(kPowerEnablePin, 1);

  // vTaskDelay(pdMS_TO_TICKS(500));

  SetupSPI();

  LV_LOG_USER("Install panel IO");
  esp_lcd_panel_io_handle_t io_handle = NULL;
  const esp_lcd_panel_io_spi_config_t io_config = {
      .cs_gpio_num = kLcdCSPin,
      .dc_gpio_num = kLcdDCPin,
      .spi_mode = 0,
      .pclk_hz = kPixelClockHz,
      .trans_queue_depth = 10,
      .lcd_cmd_bits = kLcdCmdBits,
      .lcd_param_bits = kLcdParamBits,
  };

  ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi(SPI3_HOST, &io_config, &io_handle));

  /**
   * Uncomment these lines if use custom initialization commands.
   * The array should be declared as "static const" and positioned outside the function.
   */
  // static const gc9a01_lcd_init_cmd_t lcd_init_cmds[] = {
  // //  {cmd, { data }, data_size, delay_ms}
  //     {0xfe, (uint8_t []){0x00}, 0, 0},
  //     {0xef, (uint8_t []){0x00}, 0, 0},
  //     {0xeb, (uint8_t []){0x14}, 1, 0},
  //     ...
  // };

  LV_LOG_USER("Install GC9A01 panel driver");
  esp_lcd_panel_handle_t panel_handle = NULL;
  // gc9a01_vendor_config_t vendor_config = {  // Uncomment these lines if use custom initialization commands
  //     .init_cmds = lcd_init_cmds,
  //     .init_cmds_size = sizeof(lcd_init_cmds) / sizeof(gc9a01_lcd_init_cmd_t),
  // };
  const esp_lcd_panel_dev_config_t panel_config = {
      .reset_gpio_num = kLcdResetPin,              // Set to -1 if not use
      .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_BGR,  // RGB element order
      .bits_per_pixel = 16,                        // Implemented by LCD command `3Ah` (16/18)
      // .vendor_config = &vendor_config,          // Uncomment this line if use custom initialization commands
  };
  ESP_ERROR_CHECK(esp_lcd_new_panel_gc9a01(io_handle, &panel_config, &panel_handle));
  ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_handle));
  ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle));
  ESP_ERROR_CHECK(esp_lcd_panel_invert_color(panel_handle, true));
  esp_lcd_panel_mirror(panel_handle, true, false);
  // esp_lcd_panel_swap_xy(panel_handle, true);
  ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel_handle, true));

  LV_LOG_USER("Initialize LVGL");
  lv_init();

  _display = lv_display_create(kHRes, kVRes);
  if (!_display) {
    LV_LOG_ERROR("Failed to create LVGL display");
    return ESP_ERR_INVALID_STATE;
  }
  // lv_display_set_dpi(_display, kLcdDPI);
  LV_LOG_USER("Display resolution: %dx%d", kHRes, kVRes);
  // lv_display_set_color_format(_display, LV_COLOR_FORMAT_RGB565);
  // lv_display_set_rotation(_display, LV_DISPLAY_ROTATION_180);

  LvgLBufferSetupPartial();
  lv_display_set_user_data(_display, panel_handle);
  lv_display_set_flush_cb(_display, LvglFlushCallback);

  return ESP_OK;
}

esp_err_t TouchPanelSetup() {
  if (!_default_group) {
    _default_group = lv_group_create();
    lv_group_set_default(_default_group);
  }
  EncoderSetup();
  return ESP_OK;

  ESP_ERROR_CHECK(I2cManager::GetInstance()->Probe(kTouchI2cAddress));
  LV_LOG_USER("Initialize FT3267 touch controller");

  esp_lcd_touch_config_t tp_cfg = {
      .x_max = kHRes,
      .y_max = kVRes,
      .rst_gpio_num = GPIO_NUM_NC,
      .int_gpio_num = kTouchIntPin,
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
  };
  esp_lcd_panel_io_handle_t tp_io_handle = NULL;
  esp_lcd_panel_io_i2c_config_t tp_io_config = {.dev_addr = kTouchI2cAddress,
                                                .control_phase_bytes = 1,
                                                .dc_bit_offset = 0,
                                                .lcd_cmd_bits = 8,
                                                .flags = {
                                                    .disable_control_phase = 1,
                                                }};
  // ESP_LCD_TOUCH_IO_I2C_FT5x06_CONFIG();
  tp_io_config.scl_speed_hz = I2cManager::kClockSpeedHz;

  _bus_handle = I2cManager::GetInstance()->GetBusHandle();
  ESP_RETURN_ON_ERROR(esp_lcd_new_panel_io_i2c(_bus_handle, &tp_io_config, &tp_io_handle), "M5Dial impl",
                      "Failed to create FT3267 touch IO handle");

  esp_lcd_touch_handle_t tp = NULL;
  esp_lcd_touch_new_i2c_ft5x06(tp_io_handle, &tp_cfg, &tp);

  static lv_indev_t* indev;
  indev = lv_indev_create();
  lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
  lv_indev_set_display(indev, _display);
  lv_indev_set_user_data(indev, tp);

  lv_indev_set_read_cb(indev, LvglTouchCallback);
  return ESP_OK;
}

esp_err_t EncoderSetup() {
  LV_LOG_USER("Setting up rotary encoder");

  // Configure the encoder button
  static const button_gpio_config_t encoder_btn_config = {
      .gpio_num = kEncoderBtnPin,
      .active_level = 0,
  };
  const button_config_t btn_cfg = {0};
  button_handle_t encoder_btn_handle = NULL;
  ESP_ERROR_CHECK(iot_button_new_gpio_device(&btn_cfg, &encoder_btn_config, &encoder_btn_handle));

  // Configure the rotary encoder
  const knob_config_t encoder_a_b_config = {
      .default_direction = 0,
      .gpio_encoder_a = kEncoderAPin,
      .gpio_encoder_b = kEncoderBPin,
  };
  knob_handle_t encoder_handle = iot_knob_create(&encoder_a_b_config);
  if (!encoder_handle) {
    LV_LOG_ERROR("Failed to create encoder");
    return ESP_FAIL;
  }

  // Create LVGL input device
  static lv_indev_t* indev;
  indev = lv_indev_create();
  lv_indev_set_type(indev, LV_INDEV_TYPE_ENCODER);
  lv_indev_set_display(indev, _display);
  lv_indev_set_user_data(indev, encoder_handle);

  // Set the read callback
  lv_indev_set_read_cb(indev, LvglEncoderCallback);

  // Assign to default group
  if (!_default_group) {
    _default_group = lv_group_create();
    lv_group_set_default(_default_group);
  }
  lv_indev_set_group(indev, _default_group);

  LV_LOG_USER("Encoder setup done");
  return ESP_OK;
}

// esp_err_t EncoderSetup() {
//   LV_LOG_USER("Setting up rotary encoder");
//   static const button_gpio_config_t encoder_btn_config = {
//       .gpio_num = kEncoderBtnPin,
//       .active_level = 0,
//   };

//   const knob_config_t encoder_a_b_config = {
//       .default_direction = 0,
//       .gpio_encoder_a = kEncoderAPin,
//       .gpio_encoder_b = kEncoderBPin,
//   };

//   const button_config_t btn_cfg = {0};
//   button_handle_t encoder_btn_handle = NULL;
//   ESP_ERROR_CHECK(iot_button_new_gpio_device(&btn_cfg, &encoder_btn_config, &encoder_btn_handle));

//   /* Encoder configuration structure */
//   // const lvgl_port_encoder_cfg_t encoder = {
//   // .disp = _display, .encoder_a_b = &encoder_a_b_config, .encoder_enter = encoder_btn_handle};

//   /* Add encoder input (for selected screen) */
//   // lv_indev_t* encoder_handle = lvgl_port_add_encoder(&encoder);

//   static lv_indev_t* indev;
//   indev = lv_indev_create();  // Input device driver (SetupEncoder)
//   lv_indev_set_type(indev, LV_INDEV_TYPE_ENCODER);
//   lv_indev_set_display(indev, _display);

//   lv_indev_set_read_cb(indev, LvglEncoderCallback);
//   if (!_default_group) {
//     LV_LOG_ERROR("Default group not initialized!");
//     return ESP_ERR_INVALID_STATE;
//   }
//   lv_indev_set_group(indev, _default_group);
//   LV_LOG_USER("Encoder setup done");
//   return ESP_OK;
// }

void GetDisplayDimensions(uint16_t& width, uint16_t& height) {
  width = kHRes;   // CONFIG_IMPL_LILYGO_TDISPLAY_S3_LONG_HRES;
  height = kVRes;  // CONFIG_IMPL_LILYGO_TDISPLAY_S3_LONG_VRES;
}

lv_display_t* GetDisplayObjPtr() { return _display; }

esp_err_t SetupSPI() {
  LV_LOG_USER("Initialize SPI bus");

  const spi_bus_config_t bus_config = {
      .mosi_io_num = kLcdMosiPin,
      .miso_io_num = GPIO_NUM_NC,
      .sclk_io_num = kLcdPclkPin,
      .quadwp_io_num = GPIO_NUM_NC,
      .quadhd_io_num = GPIO_NUM_NC,
      .max_transfer_sz = kMaxTransferSize,
  };

  ESP_ERROR_CHECK(spi_bus_initialize(SPI3_HOST, &bus_config, SPI_DMA_CH_AUTO));

  return ESP_OK;
}

esp_err_t LvgLBufferSetupPartial() {
  LV_LOG_USER("Assigning DMA Buffers");

  // #ifndef CONFIG_SPIRAM
  // #assert(false);  // Must have PSRAM for LVGL buffers
  // #endif

  {
    LV_LOG_USER("Allocating LVGL buffers in DMA-capable memory");
    lv_color_t* buf1 = (lv_color_t*)heap_caps_malloc(kDrawBufferSize, MALLOC_CAP_DMA);
    // assert(buf1);
    if (!buf1) {
      LV_LOG_ERROR("DMA buf1 alloc failed (bytes=%u free_dma=%u)", (unsigned)kDrawBufferSize,
                   (unsigned)heap_caps_get_free_size(MALLOC_CAP_DMA));
      return ESP_ERR_NO_MEM;
    }
    lv_color_t* buf2 = (lv_color_t*)heap_caps_malloc(kDrawBufferSize, MALLOC_CAP_DMA);
    // assert(buf2);
    if (!buf2) {
      free(buf1);
      LV_LOG_ERROR("DMA buf2 alloc failed");
      return ESP_ERR_NO_MEM;
    }

    LV_LOG_USER("Register buffers and display callback with LVGL");
    lv_display_set_buffers(_display, buf1, buf2, kDrawBufferSize, LV_DISPLAY_RENDER_MODE_PARTIAL);

    // LV_LOG_USER("buf1=%p buf2=%p expect_bytes=%u", buf1, buf2, (unsigned)(kDrawBufferSize));
    return ESP_OK;
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
}

void LvglFlushCallback(lv_display_t* disp, const lv_area_t* area, uint8_t* px_map) {
  [[maybe_unused]] uint32_t starter = esp_timer_get_time();
  LV_LOG_TRACE("LVGL Flush Callback: area x1=%li y1=%li x2=%li y2=%li", area->x1, area->y1, area->x2, area->y2);
  lv_draw_sw_rgb565_swap(px_map, lv_area_get_size(area));

  esp_lcd_panel_handle_t panel_handle = (esp_lcd_panel_handle_t)lv_display_get_user_data(disp);
  esp_lcd_panel_draw_bitmap(panel_handle, area->x1, area->y1, area->x2 + 1, area->y2 + 1, px_map);

  lv_display_flush_ready(disp);
  LV_LOG_TRACE("LVGL flush time: %lli us", (esp_timer_get_time() - starter));
}

bool LvglFlushReadyCallback(esp_lcd_panel_io_handle_t panel, esp_lcd_panel_io_event_data_t* edata, void* user_ctx) {
  // lv_display_t* disp = (lv_display_t*)user_ctx;
  LV_LOG_TRACE("LVGL flush ready callback triggered");
  lv_display_flush_ready(_display);
  return false;
};

void LvglTouchCallback(lv_indev_t* indev, lv_indev_data_t* data) {
  LV_LOG_USER(">>>> Touch Callback Called");
  uint16_t touchpad_x[1] = {0};
  uint16_t touchpad_y[1] = {0};
  uint8_t touchpad_cnt = 0;

  // Get the touch controller handle from LVGL input device user data
  esp_lcd_touch_handle_t touch_handle = (esp_lcd_touch_handle_t)lv_indev_get_user_data(indev);
  if (!touch_handle) {
    LV_LOG_ERROR("Touch handle is NULL!");
    data->state = LV_INDEV_STATE_RELEASED;
    return;
  }

  LV_LOG_USER("Reading touch data");
  // Read current touch data from the controller
  esp_err_t ret = esp_lcd_touch_read_data(touch_handle);
  if (ret != ESP_OK) {
    // If read failed, report no touch
    LV_LOG_ERROR("Touch read data failed: %s", esp_err_to_name(ret));
    data->state = LV_INDEV_STATE_RELEASED;
    return;
  }

  LV_LOG_USER("Getting touch coordinates");
  // Get coordinates and touch state
  bool touchpad_pressed = esp_lcd_touch_get_coordinates(touch_handle, touchpad_x, touchpad_y, NULL, &touchpad_cnt, 1);

  if (touchpad_pressed && touchpad_cnt > 0) {
    data->point.x = touchpad_x[0];
    data->point.y = touchpad_y[0];
    data->state = LV_INDEV_STATE_PRESSED;
    LV_LOG_USER("<<<< Touch at x=%u y=%u", (unsigned)data->point.x, (unsigned)data->point.y);
    LV_LOG_TRACE("Touch data: state=%s x=%u y=%u", data->state == LV_INDEV_STATE_PRESSED ? "PRESSED" : "RELEASED",
                 (unsigned)data->point.x, (unsigned)data->point.y);
    return;
  }
  LV_LOG_USER("<<<< No touch detected");
  data->state = LV_INDEV_STATE_RELEASED;
  return;
}

void LvglEncoderCallback(lv_indev_t* indev, lv_indev_data_t* data) {
  knob_handle_t encoder_handle = (knob_handle_t)lv_indev_get_user_data(indev);

  static int32_t last_count = 0;
  int32_t current_count = iot_knob_get_count_value(encoder_handle);

  data->enc_diff = -(current_count - last_count);
  last_count = current_count;

  data->state = (gpio_get_level(kEncoderBtnPin) == 0) ? LV_INDEV_STATE_PRESSED : LV_INDEV_STATE_RELEASED;
  LV_LOG_USER("Encoder delta=%d button=%s", (int)data->enc_diff,
              data->state == LV_INDEV_STATE_PRESSED ? "PRESSED" : "RELEASED");
  ///
  // int32_t d = __atomic_exchange_n(&_s_enc_delta, 0, __ATOMIC_RELAXED);
  // data->enc_diff = d;
  // data->state = (gpio_get_level(kEncoderBtnPin) == 0) ? LV_INDEV_STATE_PRESSED : LV_INDEV_STATE_RELEASED;
  // LV_LOG_USER("Encoder delta=%d button=%s", (int)d, data->state == LV_INDEV_STATE_PRESSED ? "PRESSED" : "RELEASED");
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
  ESP_ERROR_CHECK(esp_timer_start_once(backlight_timer, 100000));
}

void BacklightTimerCallback(void* arg) {
  ESP_ERROR_CHECK(gpio_set_direction(kLcdBackLightPin, GPIO_MODE_OUTPUT));
  gpio_set_level(kLcdBackLightPin, 1);
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