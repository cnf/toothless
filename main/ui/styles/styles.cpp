// styles.cpp
#include "styles.hpp"

namespace toothless::gui {

static uint8_t bottom_row_height = 60;

static lv_style_t btn_primary;
static lv_style_t btn_secondary;
static lv_style_t label_large;

static lv_style_t background;

static lv_style_t bottom_row;

void BaseBottomRowStyle() {
  lv_style_init(&bottom_row);
  lv_style_set_bg_color(&bottom_row, lv_color_hex(0x222222));
  lv_style_set_pad_all(&bottom_row, 10);
  lv_style_set_border_width(&bottom_row, 0);
  lv_style_set_radius(&bottom_row, 0);
  lv_style_set_layout(&bottom_row, LV_LAYOUT_FLEX);
  lv_style_set_flex_flow(&bottom_row, LV_FLEX_FLOW_ROW);
  lv_style_set_flex_align(&bottom_row, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
}

void InitBaseStyles() {
  static float mult = 0.2;
  if (kVRes < 400) {
    mult = 0.15;
  }
  static size_t bottom_row_height = lv_display_get_vertical_resolution(NULL) * mult;
  // static size_t btn_height = lv_obj_get_height(ctx.parent_screen) * mult;
  BaseBottomRowStyle();
}

void Rest() {
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
