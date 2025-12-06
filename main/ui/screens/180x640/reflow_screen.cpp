#include "ui/screens/reflow_screen.hpp"

#include <esp_timer.h>

#include <algorithm>

#include "funlog.h"
#include "heater/heater.hpp"
#include "local_helpers.hpp"
#include "ui/display/display.hpp"
#include "ui/screens/screen_helpers.hpp"
#include "ui/subjects.hpp"
#include "ui/themes/widget_factories.hpp"

extern "C" {
#include <pubsub.h>
}

namespace toothless {

lv_obj_t* ReflowScreen::Create() {
  // esp_log_level_set(FLOG_SHORT_FILENAME, ESP_LOG_DEBUG);

  // _screen = ui::CreateScreen();

  // _subjects = SubjectManager::Instance().subjects;

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

  Chart(_labels->left);
  Temperature(_labels->right);
  CreateMidSection(_labels->right);
  _labels->startstop_label = CreateBottomRow(_labels->right);
  lv_label_bind_text(_labels->startstop_label, &_subjects->start_stop, "%s");

  _update_timer = lv_timer_create(UIUpdateTimerCB, kUIUpdateIntervalMs, this);
  PS_PUB_NIL(topics::heater::profile_get);
  return _screen;
}

void ReflowScreen::Loop() {}

void ReflowScreen::UIUpdateTimerCB(lv_timer_t* timer) {
  ReflowScreen* screen = (ReflowScreen*)lv_timer_get_user_data(timer);
  if (screen) {
    screen->UpdateChart();
  }
}

}  // namespace toothless
