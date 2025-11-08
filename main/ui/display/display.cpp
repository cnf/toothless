// cSpell: words lvgl
#include "config.h"

#include "display.hpp"
#include "funlog.h"
#include "ui/display/display.hpp"
#include "ui/display/lvgl_port.hpp"
#include <driver/gpio.h>
#include <esp_check.h>
#include <esp_lcd_panel_io.h>
#include <esp_lcd_panel_ops.h>
#include <esp_lcd_panel_vendor.h>
#include <esp_lcd_st7796.h>
#include <esp_lcd_touch_xpt2046.h>
#include <esp_timer.h>

namespace toothless {
using namespace callback;

spi_host_device_t _spi_host;
std::shared_ptr<lv_display_t> _display;
// lv_display_t *_display;
/// @brief Mutex for LVGL thread safety
std::mutex _lvgl_mutex;
uint64_t _lvgl_sleep;

/// @brief  Initialize the display
/// @return true if successful, false otherwise
esp_err_t Display::Init() {
  ESP_ERROR_CHECK(SetupPanel());
  ESP_ERROR_CHECK(SetupTouchPanel());
  FLOG_DEBUG("Free heap: %u, Min free: %u", esp_get_free_heap_size(), esp_get_minimum_free_heap_size());
  ESP_RETURN_ON_FALSE(
      xTaskCreatePinnedToCore(lvgl_port_task, "LVGL", LVGL_TASK_STACK_SIZE, NULL, LVGL_TASK_PRIORITY, NULL, 0),
      ESP_ERR_INVALID_STATE, FLOG_SHORT_FILENAME, "Failed to create LVGL task");
  lvgl_boot_screen();

  {
    // Schedule backlight to turn on after 500ms (gives LVGL time to render first screen)
    esp_timer_handle_t backlight_timer = NULL;
    const esp_timer_create_args_t timer_args = {.callback = backlight_timer_cb, .arg = NULL, .name = "backlight_timer"};
    ESP_ERROR_CHECK(esp_timer_create(&timer_args, &backlight_timer));
    ESP_ERROR_CHECK(esp_timer_start_once(backlight_timer, 10000)); // 500ms in microseconds
  }
  return ESP_OK;
};

esp_err_t Display::SetupPanel() {
  // TODO: CONFIG_TL_SPI_HOST
  _spi_host = VSPI_HOST;
  FLOG_INFO("Install panel IO");
  esp_lcd_panel_io_handle_t io_handle = NULL;
  esp_lcd_panel_io_spi_config_t io_config = {
      .cs_gpio_num = CONFIG_TL_DISPLAY_SPI_CS_PIN, // BSP_SD_SPI_CS,
      .dc_gpio_num = CONFIG_TL_DISPLAY_DC_PIN,     // DISPAY_DC_GPIO,
      .spi_mode = 0,
      .pclk_hz = LCD_PIXEL_CLOCK_HZ,
      .trans_queue_depth = 10,
      .lcd_cmd_bits = LCD_CMD_BITS,
      .lcd_param_bits = LCD_PARAM_BITS,
  };
  FLOG_DEBUG("Attach the LCD IO to the SPI bus");
  ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)_spi_host, &io_config, &io_handle));
  esp_lcd_panel_handle_t panel_handle = NULL;
  esp_lcd_panel_dev_config_t panel_config = {
      .reset_gpio_num = CONFIG_TL_DISPLAY_RESET_PIN, // DISPAY_RESET_GPIO,
      .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_BGR,    // Try RGB first for ST7796S
      .bits_per_pixel = 16,
  };
  FLOG_DEBUG("Create new ST7796 panel");
  ESP_ERROR_CHECK(esp_lcd_new_panel_st7796(io_handle, &panel_config, &panel_handle));
  ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_handle));
  ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle));

  // ST7796S often needs color inversion
  ESP_ERROR_CHECK(esp_lcd_panel_invert_color(panel_handle, true));

  ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel_handle, true));

  FLOG_INFO("Initialize LVGL");
  lv_init();

  int32_t hres, vres;
  if (PORTRAIT) {
    ESP_ERROR_CHECK(esp_lcd_panel_swap_xy(panel_handle, false));
    ESP_ERROR_CHECK(esp_lcd_panel_mirror(panel_handle, true, false));
    hres = CONFIG_TL_DISPLAY_VRES;
    vres = CONFIG_TL_DISPLAY_HRES;
  } else {
    ESP_ERROR_CHECK(esp_lcd_panel_swap_xy(panel_handle, true));
    ESP_ERROR_CHECK(esp_lcd_panel_mirror(panel_handle, true, true));
    hres = CONFIG_TL_DISPLAY_HRES;
    vres = CONFIG_TL_DISPLAY_VRES;
  }

  // Create display and wrap in shared_ptr (LVGL may keep internal references)
  _display = std::shared_ptr<lv_display_t>(lv_display_create(hres, vres), [](lv_display_t *ptr) {
    // Custom deleter - check if LVGL provides a specific cleanup function
    // For now, let LVGL handle cleanup internally
  });
  if (!_display) {
    FLOG_ERROR("Failed to create LVGL display");
    return ESP_ERR_INVALID_STATE;
  }

  //

  FLOG_INFO("Display resolution: %dx%d", CONFIG_TL_DISPLAY_HRES, CONFIG_TL_DISPLAY_VRES);

  size_t draw_buffer_sz = CONFIG_TL_DISPLAY_HRES * LVGL_DRAW_BUF_LINES * sizeof(lv_color16_t);
  FLOG_INFO("Draw buffer size: %zu bytes (%d lines)", draw_buffer_sz, LVGL_DRAW_BUF_LINES);

  void *buf1 = spi_bus_dma_memory_alloc((spi_host_device_t)_spi_host, draw_buffer_sz, 0);
  if (!buf1) {
    FLOG_ERROR("DMA buf1 alloc failed");
    return ESP_ERR_NO_MEM;
  }
  void *buf2 = spi_bus_dma_memory_alloc((spi_host_device_t)_spi_host, draw_buffer_sz, 0);
  if (!buf2) {
    free(buf1);
    FLOG_ERROR("DMA buf2 alloc failed");
    return ESP_ERR_NO_MEM;
  }

  // Clear buffers to avoid garbage pixels
  memset(buf1, 0x00, draw_buffer_sz);
  memset(buf2, 0x00, draw_buffer_sz);

  lv_display_flush_ready(_display.get()); // trigger LVGL flush

  FLOG_INFO("Allocated DMA buffers: buf1=%p, buf2=%p", buf1, buf2);
  // initialize LVGL draw buffers
  lv_display_set_buffers(_display.get(), buf1, buf2, draw_buffer_sz, LV_DISPLAY_RENDER_MODE_PARTIAL);
  // associate the mipi panel handle to the display
  lv_display_set_user_data(_display.get(), panel_handle);
  // set color depth
  lv_display_set_color_format(_display.get(), LV_COLOR_FORMAT_RGB565);
  // set the callback which can copy the rendered image to an area of the display
  lv_display_set_flush_cb(_display.get(), lvgl_flush_cb);

  FLOG_INFO("Install LVGL tick timer");
  // Tick interface for LVGL (using esp_timer to generate 2ms periodic event)
  const esp_timer_create_args_t lvgl_tick_timer_args = {.callback = &lvgl_increase_tick, .name = "lvgl_tick"};
  esp_timer_handle_t lvgl_tick_timer = NULL;
  ESP_ERROR_CHECK(esp_timer_create(&lvgl_tick_timer_args, &lvgl_tick_timer));
  ESP_ERROR_CHECK(esp_timer_start_periodic(lvgl_tick_timer, LVGL_TICK_PERIOD_MS * 1000));

  FLOG_INFO("Register io panel event callback for LVGL flush ready notification");
  const esp_lcd_panel_io_callbacks_t cbs = {
      .on_color_trans_done = lvgl_notify_flush_ready,
  };
  /* Register done callback */
  ESP_ERROR_CHECK(esp_lcd_panel_io_register_event_callbacks(io_handle, &cbs, _display.get()));
  // lv_display_add_event_cb(_display.get(), lvgl_display_event_cb, LV_EVENT_REFR_READY, NULL);

  return ESP_OK;
}

/// @brief  Setup the touch panel
/// @return
esp_err_t Display::SetupTouchPanel() {
  bool swapxy, mirror_x, mirror_y;
  uint16_t hres, vres;
  if (PORTRAIT) {
    swapxy = false;
    mirror_x = 0;
    mirror_y = 0;
    hres = CONFIG_TL_DISPLAY_VRES;
    vres = CONFIG_TL_DISPLAY_HRES;
  } else {
    swapxy = true;
    mirror_x = 0;
    mirror_y = 1;
    hres = CONFIG_TL_DISPLAY_VRES;
    vres = CONFIG_TL_DISPLAY_HRES;
  }
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
              .mirror_y = mirror_y, // CONFIG_EXAMPLE_LCD_MIRROR_Y,
          },
  };
  esp_lcd_touch_handle_t tp = NULL;

  FLOG_INFO("Initialize touch controller XPT2046");
  ESP_ERROR_CHECK(esp_lcd_touch_new_spi_xpt2046(tp_io_handle, &tp_cfg, &tp));
  static lv_indev_t *indev;
  indev = lv_indev_create(); // Input device driver (SetupTouchPanel)
  lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
  lv_indev_set_display(indev, _display.get());
  lv_indev_set_user_data(indev, tp);

  lv_indev_set_read_cb(indev, lvgl_touch_cb);
  return ESP_OK;
}

std::mutex &Display::GetLvglMutex() { return _lvgl_mutex; }

lv_display_t *Display::GetDisplayPtr() { return _display.get(); }

} // namespace toothless