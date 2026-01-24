// cSpell: words lvgl qspi
#include "display_impl.hpp"

#include <driver/gpio.h>
#include <esp_check.h>
#include <esp_lcd_panel_io.h>
#include <esp_lcd_panel_io_additions.h>
#include <esp_lcd_panel_ops.h>
#include <esp_lcd_panel_rgb.h>
#include <esp_lcd_panel_vendor.h>
#include <esp_lcd_st7701.h>
#include <esp_lcd_touch_gt911.h>
#include <esp_timer.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

#include <map>

#include "funlog.h"
#include "i2c_manager.hpp"
#include "impl_expander.hpp"
#include "implementation.hpp"
#include "sdkconfig.h"

namespace impl {
namespace display {

spi_host_device_t _spi_host;
lv_display_t* _display;
uint64_t _lvgl_sleep;

i2c_master_dev_handle_t _dev_handle;
i2c_master_bus_handle_t _bus_handle;

static const st7701_lcd_init_cmd_t lcd_init_cmds[] = {
    //  {cmd, { data }, data_size, delay_ms}
    {0x11, (uint8_t[]){0x00}, 0, 120},
    {0xFF, (uint8_t[]){0x77, 0x01, 0x00, 0x00, 0x10}, 5, 0},
    {0xC0, (uint8_t[]){0x3B, 0x00}, 2, 0},
    {0xC1, (uint8_t[]){0x0D, 0x02}, 2, 0},
    {0xC2, (uint8_t[]){0x21, 0x08}, 2, 0},
    {0xCD, (uint8_t[]){0x08}, 1, 0},
    {0xB0, (uint8_t[]){0x00, 0x11, 0x18, 0x0E, 0x11, 0x06, 0x07, 0x08, 0x07, 0x22, 0x04, 0x12, 0x0F, 0xAA, 0x31, 0x18},
     16, 0},
    {0xB1, (uint8_t[]){0x00, 0x11, 0x19, 0x0E, 0x12, 0x07, 0x08, 0x08, 0x08, 0x22, 0x04, 0x11, 0x11, 0xA9, 0x32, 0x18},
     16, 0},
    {0xFF, (uint8_t[]){0x77, 0x01, 0x00, 0x00, 0x11}, 5, 0},
    {0xB0, (uint8_t[]){0x60}, 1, 0},
    {0xB1, (uint8_t[]){0x30}, 1, 0},
    {0xB2, (uint8_t[]){0x87}, 1, 0},
    {0xB3, (uint8_t[]){0x80}, 1, 0},
    {0xB5, (uint8_t[]){0x49}, 1, 0},
    {0xB7, (uint8_t[]){0x85}, 1, 0},
    {0xB8, (uint8_t[]){0x21}, 1, 0},
    {0xC1, (uint8_t[]){0x78}, 1, 0},
    {0xC2, (uint8_t[]){0x78}, 1, 20},
    {0xE0, (uint8_t[]){0x00, 0x1B, 0x02}, 3, 0},
    {0xE1, (uint8_t[]){0x08, 0xA0, 0x00, 0x00, 0x07, 0xA0, 0x00, 0x00, 0x00, 0x44, 0x44}, 11, 0},
    {0xE2, (uint8_t[]){0x11, 0x11, 0x44, 0x44, 0xED, 0xA0, 0x00, 0x00, 0xEC, 0xA0, 0x00, 0x00}, 12, 0},
    {0xE3, (uint8_t[]){0x00, 0x00, 0x11, 0x11}, 4, 0},
    {0xE4, (uint8_t[]){0x44, 0x44}, 2, 0},
    {0xE5, (uint8_t[]){0x0A, 0xE9, 0xD8, 0xA0, 0x0C, 0xEB, 0xD8, 0xA0, 0x0E, 0xED, 0xD8, 0xA0, 0x10, 0xEF, 0xD8, 0xA0},
     16, 0},
    {0xE6, (uint8_t[]){0x00, 0x00, 0x11, 0x11}, 4, 0},
    {0xE7, (uint8_t[]){0x44, 0x44}, 2, 0},
    {0xE8, (uint8_t[]){0x09, 0xE8, 0xD8, 0xA0, 0x0B, 0xEA, 0xD8, 0xA0, 0x0D, 0xEC, 0xD8, 0xA0, 0x0F, 0xEE, 0xD8, 0xA0},
     16, 0},
    {0xEB, (uint8_t[]){0x02, 0x00, 0xE4, 0xE4, 0x88, 0x00, 0x40}, 7, 0},
    {0xEC, (uint8_t[]){0x3C, 0x00}, 2, 0},
    {0xED, (uint8_t[]){0xAB, 0x89, 0x76, 0x54, 0x02, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x20, 0x45, 0x67, 0x98, 0xBA},
     16, 0},
    {0xFF, (uint8_t[]){0x77, 0x01, 0x00, 0x00, 0x00}, 5, 0},
    {0x36, (uint8_t[]){0x00}, 1, 0},
    {0x3A, (uint8_t[]){0x66}, 1, 0},
    {0x21, (uint8_t[]){0x00}, 0, 120},
    {0x29, (uint8_t[]){0x00}, 0, 0},
};

esp_err_t DisplayPanelSetup() {
  impl::expander::ExpanderSetup();
  auto io_expander = impl::expander::GetExpanderHandle();

  esp_io_expander_set_dir(io_expander, EXP_SYS_EN | EXP_BEE_EN | EXP_LCD_RST | EXP_LCD_TOUCH_RST, IO_EXPANDER_OUTPUT);
  esp_io_expander_set_dir(io_expander, EXP_RTC_INT, IO_EXPANDER_INPUT);
  esp_io_expander_set_level(io_expander, EXP_BEE_EN | EXP_LCD_RST | EXP_LCD_TOUCH_RST, 0);
  vTaskDelay(pdMS_TO_TICKS(200));
  esp_io_expander_set_level(io_expander, EXP_SYS_EN | EXP_LCD_RST | EXP_LCD_TOUCH_RST, 1);
  vTaskDelay(pdMS_TO_TICKS(200));
  // {
  // ESP_ERROR_CHECK(gpio_set_direction(kSystemEnablePin, GPIO_MODE_OUTPUT));
  // ESP_ERROR_CHECK(gpio_set_direction(kBeeperEnablePin, GPIO_MODE_OUTPUT));
  // ESP_ERROR_CHECK(gpio_set_direction(kLcdResetPin, GPIO_MODE_OUTPUT));
  // ESP_ERROR_CHECK(gpio_set_direction(kLcdTouchResetPin, GPIO_MODE_OUTPUT));
  // ESP_ERROR_CHECK(gpio_set_direction(kRtcInterruptPin, GPIO_MODE_INPUT));
  // ESP_ERROR_CHECK(gpio_set_level(kBeeperEnablePin, 0));
  // ESP_ERROR_CHECK(gpio_set_level(kLcdResetPin, 0));
  // ESP_ERROR_CHECK(gpio_set_level(kLcdTouchResetPin, 0));
  // vTaskDelay(pdMS_TO_TICKS(200));
  // ESP_ERROR_CHECK(gpio_set_level(kSystemEnablePin, 1));
  // ESP_ERROR_CHECK(gpio_set_level(kLcdResetPin, 1));
  // ESP_ERROR_CHECK(gpio_set_level(kLcdTouchResetPin, 1));
  // vTaskDelay(pdMS_TO_TICKS(200));
  // }

  LV_LOG_USER("Install 3-wire SPI panel IO");
  spi_line_config_t line_config = {
      .cs_io_type = IO_TYPE_GPIO,
      .cs_gpio_num = kLcdSPICSPin,
      .scl_io_type = IO_TYPE_GPIO,
      .scl_gpio_num = kLcdSPISclkPin,
      .sda_io_type = IO_TYPE_GPIO,
      .sda_gpio_num = kLcdSPISdaPin,
  };
  esp_lcd_panel_io_3wire_spi_config_t io_config = ST7701_PANEL_IO_3WIRE_SPI_CONFIG(line_config, 0);
  esp_lcd_panel_io_handle_t io_handle = NULL;
  ESP_ERROR_CHECK(esp_lcd_new_panel_io_3wire_spi(&io_config, &io_handle));

  LV_LOG_USER("Install ST7701 panel driver");
  esp_lcd_rgb_panel_config_t rgb_config = {
      .clk_src = LCD_CLK_SRC_DEFAULT,
      .timings =
          {
              .pclk_hz = kPixelClockHz,
              .h_res = kHRes,
              .v_res = kVRes,
              .hsync_pulse_width = 10,
              .hsync_back_porch = 10,
              .hsync_front_porch = 20,
              .vsync_pulse_width = 10,
              .vsync_back_porch = 10,
              .vsync_front_porch = 10,
              .flags = {.pclk_active_neg = false},
          },
      .data_width = kLcdDataWidth,
      .bits_per_pixel = kLcdBitsPerPixel,
      .num_fbs = 1,
      .bounce_buffer_size_px = kBounceBuffer,  // kDisplayBufferPixels,
      .psram_trans_align = 64,
      .hsync_gpio_num = kLcdHSPin,
      .vsync_gpio_num = kLcdVSPin,
      .de_gpio_num = kLcdDEPin,
      .pclk_gpio_num = kLcdPCLKPin,
      .disp_gpio_num = GPIO_NUM_NC,
      .data_gpio_nums =
          {
              kLcdData0Pin,
              kLcdData1Pin,
              kLcdData2Pin,
              kLcdData3Pin,
              kLcdData4Pin,
              kLcdData5Pin,
              kLcdData6Pin,
              kLcdData7Pin,
              kLcdData8Pin,
              kLcdData9Pin,
              kLcdData10Pin,
              kLcdData11Pin,
              kLcdData12Pin,
              kLcdData13Pin,
              kLcdData14Pin,
              kLcdData15Pin,
          },
      .flags = {.fb_in_psram = 1},
  };

  st7701_vendor_config_t vendor_config = {
      .init_cmds = lcd_init_cmds,  // Uncomment these line if use custom initialization commands
      .init_cmds_size = sizeof(lcd_init_cmds) / sizeof(st7701_lcd_init_cmd_t),
      .rgb_config = &rgb_config,
      .flags =
          {
              .mirror_by_cmd = 1,       // Only work when `enable_io_multiplex` is set to 0
              .enable_io_multiplex = 0, /**
                                         * Set to 1 if panel IO is no longer needed after LCD initialization.
                                         * If the panel IO pins are sharing other pins of the RGB interface to save
                                         * GPIOs, Please set it to 1 to release the pins.
                                         */
          },
  };

  const esp_lcd_panel_dev_config_t panel_config = {
      .reset_gpio_num = GPIO_NUM_NC,               // Set to -1 if not use
      .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB,  // Implemented by LCD command `36h`
      .bits_per_pixel = kLcdBitsPerPixel,          // Implemented by LCD command `3Ah` (16/18/24)
      .vendor_config = &vendor_config,
  };
  esp_lcd_panel_handle_t panel_handle = NULL;
  /**
   * Only create RGB when `enable_io_multiplex`
   * is set to 0, or initialize st7701 meanwhile
   */
  ESP_ERROR_CHECK(esp_lcd_new_panel_st7701(io_handle, &panel_config, &panel_handle));
  // Only reset RGB when `enable_io_multiplex` is set to 1, or reset st7701 meanwhile
  ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_handle));
  // Only initialize RGB when `enable_io_multiplex` is set to 1, or initialize st7701 meanwhile
  ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle));

  LV_LOG_USER("Clock speed: %d MHz", (int)kPixelClockHz / 1000000);

  ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel_handle, true));

  // // ESP_ERROR_CHECK(esp_lcd_panel_invert_color(panel_handle, true));

  LV_LOG_USER("Initialize LVGL");
  lv_init();

  _display = lv_display_create(kHRes, kVRes);
  if (!_display) {
    LV_LOG_ERROR("Failed to create LVGL display");
    return ESP_ERR_INVALID_STATE;
  }
  lv_display_set_dpi(_display, kLcdDPI);
  LV_LOG_USER("Display resolution: %lix%li, %i DPI", kHRes, kVRes, kLcdDPI);
  // lv_display_set_rotation(_display, LV_DISPLAY_ROTATION_90);
  // esp_lcd_panel_mirror(panel_handle, false, true);

  // esp_lcd_panel_swap_xy(panel_handle, true);

  {
    LV_LOG_USER("Allocating LVGL buffers from PSRAM");
    void* buf1 = heap_caps_malloc(kDisplayBufferBytes, MALLOC_CAP_SPIRAM);
    assert(buf1);
    void* buf2 = heap_caps_malloc(kDisplayBufferBytes, MALLOC_CAP_SPIRAM);
    assert(buf2);

    LV_LOG_USER("Register buffers and display callback with LVGL");
    lv_display_set_buffers(_display, buf1, buf2, kDisplayBufferBytes, LV_DISPLAY_RENDER_MODE_PARTIAL);
    // lv_display_set_buffers(_display, buf1, buf2, kLcdHRes * kLcdVRes / 10, LV_DISPLAY_RENDER_MODE_FULL);
    lv_display_set_user_data(_display, panel_handle);
    lv_display_set_flush_cb(_display, LvglFlushCallback);
    LV_LOG_USER("buf1=%p buf2=%p expect_bytes=%u", buf1, buf2, (unsigned)(kDisplayBufferBytes));
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
}  // namespace display

esp_err_t TouchPanelSetup() {
  LV_LOG_USER("Setup Touch Panel");
  i2c_device_config_t i2c_dev_conf = {
      .dev_addr_length = I2C_ADDR_BIT_LEN_7,
      .device_address = kTouchI2cAddress,
      .scl_speed_hz = I2cManager::kClockSpeedHz,
  };
  esp_err_t err = I2cManager::GetInstance()->AddDevice(&i2c_dev_conf, &_dev_handle);
  if (err != ESP_OK) {
    LV_LOG_ERROR("Failed to add touch device: %s", esp_err_to_name(err));
    return err;
  }
  _bus_handle = I2cManager::GetInstance()->GetBusHandle();

  esp_lcd_panel_io_handle_t tp_io_handle = nullptr;
  esp_lcd_panel_io_i2c_config_t tp_io_config = ESP_LCD_TOUCH_IO_I2C_GT911_CONFIG();
  tp_io_config.scl_speed_hz = I2cManager::kClockSpeedHz;

  esp_lcd_touch_io_gt911_config_t tp_gt911_config = {
      .dev_addr = kTouchI2cAddress,
  };

  esp_lcd_touch_config_t tp_cfg = {
      .x_max = ::impl::kHRes,
      .y_max = ::impl::kVRes,
      .rst_gpio_num = GPIO_NUM_NC,
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

esp_err_t BacklightSetup() {
  // ::impl::ExpanderSetup();
  auto io_expander = impl::expander::GetExpanderHandle();
  esp_io_expander_set_dir(io_expander, EXP_LCD_BACKLIGHT_PIN, IO_EXPANDER_OUTPUT);
  esp_io_expander_set_level(io_expander, EXP_LCD_BACKLIGHT_PIN, 1);
  return impl::expander::SetPWM(255);
  // gpio_set_direction((gpio_num_t)kLcdBacklightPin, GPIO_MODE_OUTPUT);
  // gpio_set_level((gpio_num_t)kLcdBacklightPin, 1);
  // return ESP_OK;
};

esp_err_t SetBrightness(uint8_t brightness) {
  // FLOG_WARN("Brightness controll not available for this display");
  return impl::expander::SetPWM(brightness);
  //  return ESP_OK;
};

void GetDisplayDimensions(uint16_t& width, uint16_t& height) {
  // if (PORTRAIT) {
  //   width = IMPL_LILYGO_TDISPLAY_S3_LONG_VRES;
  //   height = IMPL_LILYGO_TDISPLAY_S3_LONG_HRES;
  // } else {
  width = ::impl::kHRes;
  height = ::impl::kVRes;
  // }
}

lv_display_t* GetDisplayObjPtr() { return _display; }

esp_err_t SetupQSPI() { return ESP_OK; }

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
  *x = map(*x, TOUCH_H_RES_MIN, TOUCH_H_RES_MAX, 0, ::impl::kHRes);
  *y = map(*y, TOUCH_V_RES_MIN, TOUCH_V_RES_MAX, 0, ::impl::kVRes);
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
  lv_async_call(
      [](void*) {
        ShowBootScreen();
        // Backlight();
        BacklightSetup();
      },
      nullptr);
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
  lv_screen_load(boot_scr);
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
  // FIXME: port expander backlight control
  // ESP_ERROR_CHECK(gpio_set_direction(kLcdBacklightPin, GPIO_MODE_OUTPUT));
  // gpio_set_level(kLcdBacklightPin, 1);
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

}  // namespace display
}  // namespace impl