#include "ui/screens/autotune_screen.hpp"

#include <cmath>

#include "funlog.h"
#include "heater/heater.hpp"
#include "ui/display/display.hpp"
#include "ui/screens/screen_helpers.hpp"
#include "ui/themes/style_registry.hpp"
#include "ui/themes/widget_factories.hpp"

extern "C" {
#include <pubsub.h>
}

namespace toothless {

lv_obj_t* AutotuneScreen::Create() {
  esp_log_level_set(FLOG_SHORT_FILENAME, ESP_LOG_DEBUG);

  // lv_obj_set_layout(_screen, LV_LAYOUT_FLEX);
  // lv_obj_set_flex_flow(_screen, LV_FLEX_FLOW_COLUMN);

  _labels->left = ui::CreateSubScreen(_screen);
  lv_obj_set_size(_labels->left, lv_pct(50), lv_pct(100));

  _labels->right = ui::CreateSubScreen(_screen);
  lv_obj_set_size(_labels->right, lv_pct(50), lv_pct(100));
  lv_obj_set_style_pad_gap(_labels->right, 0, 0);

  // lv_obj_set_style_border_width(_labels->right, 1, 0);
  // lv_obj_set_style_border_color(_labels->right, lv_color_hex(0x999900), 0);

  lv_obj_set_layout(_labels->right, LV_LAYOUT_FLEX);
  lv_obj_set_flex_flow(_labels->right, LV_FLEX_FLOW_COLUMN);

  lv_obj_set_x(_labels->right, lv_pct(50));

  Title(_labels->right);
  Temperature(_labels->right);
  Chart(_labels->left);

  CreateBottomRow(_labels->right);

  _update_timer = lv_timer_create(UIUpdateTimerCB, kUIUpdateIntervalMs, this);

  return _screen;
}
void AutotuneScreen::Loop() {}

}  // namespace toothless
