// styles.cpp
#include "styles.hpp"

namespace toothless::gui {

lv_style_t btn_primary;
lv_style_t btn_secondary;
lv_style_t label_large;

void init_styles() {
  lv_style_init(&btn_primary);
  lv_style_set_bg_color(&btn_primary, lv_color_hex(0x3498db));
  lv_style_set_radius(&btn_primary, 6);
  lv_style_set_text_color(&btn_primary, lv_color_white());
  lv_style_set_pad_all(&btn_primary, 8);

  lv_style_init(&btn_secondary);
  lv_style_set_bg_color(&btn_secondary, lv_color_hex(0x95a5a6));
  lv_style_set_radius(&btn_secondary, 6);
  lv_style_set_text_color(&btn_secondary, lv_color_white());
  lv_style_set_pad_all(&btn_secondary, 8);

  lv_style_init(&label_large);
  lv_style_set_text_font(&label_large, &lv_font_montserrat_22);
}

}  // namespace toothless::gui
