#include "ui/screens/reflow_screen.hpp"

#include <esp_timer.h>

#include <algorithm>

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
lv_obj_t* ReflowScreen::Create() {
  // esp_log_level_set(FLOG_SHORT_FILENAME, ESP_LOG_DEBUG);

  lv_obj_set_layout(_screen, LV_LAYOUT_FLEX);
  lv_obj_set_flex_flow(_screen, LV_FLEX_FLOW_COLUMN);

  Temperature(_screen);
  Chart(_screen);
  CreateMidSection(_screen);
  _labels->startstop_label = CreateBottomRow(_screen);

  _update_timer = lv_timer_create(UIUpdateTimerCB, kUIUpdateIntervalMs, this);
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
