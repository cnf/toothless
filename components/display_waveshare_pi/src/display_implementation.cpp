// cSpell: words lvgl
#include "display_implementation.hpp"

#include <driver/gpio.h>
#include <esp_check.h>
#include <esp_lcd_panel_ops.h>
#include <esp_lcd_panel_vendor.h>
#include <esp_lcd_st7796.h>
#include <esp_lcd_touch_xpt2046.h>
#include <esp_timer.h>

#include "funlog.h"
#include "sdkconfig.h"
// #include "theme.hpp"

namespace display {
namespace impl {

spi_host_device_t _spi_host;
lv_display_t* _display;
uint64_t _lvgl_sleep;

// esp_err_t DisplayImplementation::Init() {
//   ESP_ERROR_CHECK(SetupPanel());
//   ESP_ERROR_CHECK(SetTheme(_display));
//   ESP_ERROR_CHECK(SetupTouchPanel());
//   FLOG_DEBUG("Free heap: %u, Min free: %u", esp_get_free_heap_size(), esp_get_minimum_free_heap_size());
//   ESP_RETURN_ON_FALSE(
//       xTaskCreatePinnedToCore(lvgl_port_task, "LVGL", LVGL_TASK_STACK_SIZE, NULL, LVGL_TASK_PRIORITY, NULL, 0),
//       ESP_ERR_INVALID_STATE, FLOG_SHORT_FILENAME, "Failed to create LVGL task");
//   lvgl_boot_screen();

//   {
//     // Schedule backlight to turn on after 500ms (gives LVGL time to render first screen)
//     esp_timer_handle_t backlight_timer = NULL;
//     const esp_timer_create_args_t timer_args = {.callback = backlight_timer_cb, .arg = NULL, .name =
//     "backlight_timer"}; ESP_ERROR_CHECK(esp_timer_create(&timer_args, &backlight_timer));
//     ESP_ERROR_CHECK(esp_timer_start_once(backlight_timer, 100000));  // 500ms in microseconds
//   }
//   return ESP_OK;
// };

esp_err_t DisplayPanelSetup() {
  // TODO: CONFIG_TL_SPI_HOST
  // _spi_host = (spi_host_device_t)CONFIG_IMPL_DISPLAY_SPI_HOST;
  _spi_host = VSPI_HOST;  // TODO: make configurable
  LV_LOG_INFO("Install panel IO");
  esp_lcd_panel_io_handle_t io_handle = NULL;
  esp_lcd_panel_io_spi_config_t io_config = {
      .cs_gpio_num = CONFIG_TL_DISPLAY_SPI_CS_PIN,  // BSP_SD_SPI_CS,
      .dc_gpio_num = CONFIG_TL_DISPLAY_DC_PIN,      // DISPAY_DC_GPIO,
      .spi_mode = 0,
      .pclk_hz = kLcdPixelClockHz,
      .trans_queue_depth = 10,
      .lcd_cmd_bits = kLcdCmdBits,
      .lcd_param_bits = kLcdParamBits,
  };
  LV_LOG_INFO("Attach the LCD IO to the SPI bus");
  ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)_spi_host, &io_config, &io_handle));
  esp_lcd_panel_handle_t panel_handle = NULL;
  esp_lcd_panel_dev_config_t panel_config = {
      .reset_gpio_num = CONFIG_TL_DISPLAY_RESET_PIN,  // DISPAY_RESET_GPIO,
      .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_BGR,     // Try RGB first for ST7796S
      .bits_per_pixel = 16,
  };
  LV_LOG_INFO("Create new ST7796 panel");
  ESP_ERROR_CHECK(esp_lcd_new_panel_st7796(io_handle, &panel_config, &panel_handle));
  ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_handle));
  ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle));

  // ST7796S often needs color inversion
  ESP_ERROR_CHECK(esp_lcd_panel_invert_color(panel_handle, true));

  ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel_handle, true));

  LV_LOG_INFO("Initialize LVGL");
  lv_init();

  // TODO: lets not do this here, but figure out how to make it configurable?
  int32_t hres, vres;
  // if (PORTRAIT) {
  //   ESP_ERROR_CHECK(esp_lcd_panel_swap_xy(panel_handle, false));
  //   ESP_ERROR_CHECK(esp_lcd_panel_mirror(panel_handle, true, false));
  //   hres = CONFIG_TL_DISPLAY_VRES;
  //   vres = CONFIG_TL_DISPLAY_HRES;
  // } else {
  ESP_ERROR_CHECK(esp_lcd_panel_swap_xy(panel_handle, true));
  ESP_ERROR_CHECK(esp_lcd_panel_mirror(panel_handle, true, true));
  hres = CONFIG_TL_DISPLAY_HRES;
  vres = CONFIG_TL_DISPLAY_VRES;
  // }

  // Create display and wrap in shared_ptr (LVGL may keep internal references)
  _display = lv_display_create(hres, vres), [](lv_display_t* ptr) {
    // FIXME: custom deleter?
    // Custom deleter - check if LVGL provides a specific cleanup function
    // For now, let LVGL handle cleanup internally
  };
  if (!_display) {
    LV_LOG_ERROR("Failed to create LVGL display");
    return ESP_ERR_INVALID_STATE;
  }

  LV_LOG_INFO("Display resolution: %dx%d", hres, vres);

  size_t draw_buffer_sz = CONFIG_TL_DISPLAY_HRES * kLvglDrawBufferLines * sizeof(lv_color16_t);
  LV_LOG_INFO("Draw buffer size: %zu bytes (%d lines)", draw_buffer_sz, kLvglDrawBufferLines);

  void* buf1 = spi_bus_dma_memory_alloc((spi_host_device_t)_spi_host, draw_buffer_sz, 0);
  if (!buf1) {
    LV_LOG_ERROR("DMA buf1 alloc failed");
    return ESP_ERR_NO_MEM;
  }
  void* buf2 = spi_bus_dma_memory_alloc((spi_host_device_t)_spi_host, draw_buffer_sz, 0);
  if (!buf2) {
    free(buf1);
    LV_LOG_ERROR("DMA buf2 alloc failed");
    return ESP_ERR_NO_MEM;
  }

  // Clear buffers to avoid garbage pixels
  memset(buf1, 0x00, draw_buffer_sz);
  memset(buf2, 0x00, draw_buffer_sz);

  LV_LOG_INFO("Allocated DMA buffers: buf1=%p, buf2=%p", buf1, buf2);
  // initialize LVGL draw buffers
  lv_display_set_buffers(_display, buf1, buf2, draw_buffer_sz, LV_DISPLAY_RENDER_MODE_PARTIAL);
  // associate the mipi panel handle to the display
  lv_display_set_user_data(_display, panel_handle);
  // set color depth
  lv_display_set_color_format(_display, LV_COLOR_FORMAT_RGB565);
  // set the callback which can copy the rendered image to an area of the display
  lv_display_set_flush_cb(_display, LvglFlushCallback);
  {
    LV_LOG_INFO("Register io panel event callback for LVGL flush ready notification");
    const esp_lcd_panel_io_callbacks_t cbs = {
        .on_color_trans_done = LvglFlushReadyCallback,
    };

    /* Register done callback */
    ESP_ERROR_CHECK(esp_lcd_panel_io_register_event_callbacks(io_handle, &cbs, _display));
    // lv_display_add_event_cb(_display, lvgl_display_event_cb, LV_EVENT_REFR_READY, NULL);
  }
  // lv_display_flush_ready(_display);  // trigger LVGL flush

  return ESP_OK;
}

esp_err_t TouchPanelSetup() {
  bool swapxy, mirror_x, mirror_y;
  uint16_t hres, vres;
  // if (PORTRAIT) {
  //   swapxy = false;
  //   mirror_x = 0;
  //   mirror_y = 0;
  //   hres = CONFIG_TL_DISPLAY_VRES;
  //   vres = CONFIG_TL_DISPLAY_HRES;
  // } else {
  swapxy = true;
  mirror_x = 0;
  mirror_y = 1;
  hres = CONFIG_TL_DISPLAY_VRES;
  vres = CONFIG_TL_DISPLAY_HRES;
  // }

  esp_lcd_panel_io_handle_t tp_io_handle = NULL;
  esp_lcd_panel_io_spi_config_t tp_io_config = ESP_LCD_TOUCH_IO_SPI_XPT2046_CONFIG(CONFIG_TL_TOUCHPANEL_SPI_CS_PIN);
  ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)_spi_host, &tp_io_config, &tp_io_handle));
  esp_lcd_touch_config_t tp_cfg = {
      .x_max = hres,
      .y_max = vres,
      .rst_gpio_num = GPIO_NUM_NC,
      .int_gpio_num = (gpio_num_t)CONFIG_TL_TOUCHPANEL_INT_PIN,
      .flags =
          {
              .swap_xy = swapxy,
              .mirror_x = mirror_x,
              .mirror_y = mirror_y,  // CONFIG_EXAMPLE_LCD_MIRROR_Y,
          },
  };
  esp_lcd_touch_handle_t tp = NULL;

  LV_LOG_INFO("Initialize touch controller XPT2046");
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
  // if (PORTRAIT) {
  //   width = IMPL_DISPLAY_VRES;
  //   height = IMPL_DISPLAY_HRES;
  // } else {
  width = CONFIG_IMPL_DISPLAY_HRES;
  height = CONFIG_IMPL_DISPLAY_VRES;
  // }
}

lv_display_t* GetDisplayObjPtr() { return _display; }

void LvglFlushCallback(lv_display_t* disp, const lv_area_t* area, uint8_t* px_map) {
  uint32_t starter = esp_timer_get_time();

  esp_lcd_panel_handle_t panel_handle = (esp_lcd_panel_handle_t)lv_display_get_user_data(disp);
  int offsetx1 = area->x1;
  int offsetx2 = area->x2;
  int offsety1 = area->y1;
  int offsety2 = area->y2;

  // because SPI LCD is big-endian, we need to swap the RGB bytes order
  lv_draw_sw_rgb565_swap(px_map, (offsetx2 + 1 - offsetx1) * (offsety2 + 1 - offsety1));

  // copy a buffer's content to a specific area of the display
  esp_lcd_panel_draw_bitmap(panel_handle, offsetx1, offsety1, offsetx2 + 1, offsety2 + 1, px_map);

  // CRITICAL: Tell LVGL that flushing is done
  lv_display_flush_ready(disp);
  LV_LOG_INFO("LVGL flush time: %lli us", (esp_timer_get_time() - starter));
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
  LV_LOG_INFO("Displaying boot screen");
  lv_lock();

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
  lv_unlock();
  Backlight();
}

void Backlight() {
  // Schedule backlight to turn on after 500ms (gives LVGL time to render first screen)
  esp_timer_handle_t backlight_timer = NULL;
  const esp_timer_create_args_t timer_args = {
      .callback = BacklightTimerCallback, .arg = NULL, .name = "backlight_timer"};
  ESP_ERROR_CHECK(esp_timer_create(&timer_args, &backlight_timer));
  ESP_ERROR_CHECK(esp_timer_start_once(backlight_timer, 100000));  // 500ms in microseconds
}

void BacklightTimerCallback(void* arg) {
  ESP_ERROR_CHECK(gpio_set_direction((gpio_num_t)CONFIG_IMPL_DISPLAY_BACKLIGHT_PIN, GPIO_MODE_OUTPUT));
  gpio_set_level((gpio_num_t)CONFIG_IMPL_DISPLAY_BACKLIGHT_PIN, 1);  // TODO: make configurable
};

}  // namespace impl
}  // namespace display