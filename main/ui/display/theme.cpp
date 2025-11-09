#include "theme.hpp"

#include <lvgl.h>

namespace toothless {
static void theme_apply_cb(lv_theme_t* th, lv_obj_t* obj) {
  LV_UNUSED(th);
  // theme_apply(th, obj);
  // Only override screen objects
  if (lv_obj_check_type(obj, &lv_obj_class)) {
    lv_obj_set_style_bg_color(obj, lv_color_hex(0x001122), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(obj, LV_OPA_COVER, LV_PART_MAIN);
  }
}

esp_err_t SetTheme(lv_display_t* display) {
  // Initialize the default theme. Use a clear, readable palette and the built-in fonts.
  theme = lv_theme_default_init(display,
                                /*Primary*/ lv_color_hex(0xFF6700),
                                /*Secondary*/ lv_color_hex(0x0012FF),
                                /*Dark mode?*/ true,
                                /*Font*/ &lv_font_montserrat_18);

  // lv_theme_set_apply_cb(theme, theme_apply_cb);

  // lv_style_set_bg_color(lv_theme_get_style(theme, LV_THEME_DEF_SCR, LV_PART_MAIN), lv_color_hex(0x001122));
  lv_display_set_theme(display, theme);

  // lv_obj_t* layer = lv_display_get_layer_bottom(display);
  // if (layer) {
  //   lv_obj_set_style_bg_color(layer, lv_color_hex(0x001122), 0);
  //   lv_obj_set_style_bg_opa(layer, LV_OPA_COVER, 0);
  // }

  return ESP_OK;
}
}  // namespace toothless