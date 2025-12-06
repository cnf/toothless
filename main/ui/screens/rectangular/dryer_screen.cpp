#include "ui/screens/dryer_screen.hpp"

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

lv_obj_t* DryerScreen::Create() {
  esp_log_level_set(FLOG_SHORT_FILENAME, ESP_LOG_DEBUG);

  lv_obj_set_layout(_screen, LV_LAYOUT_FLEX);
  lv_obj_set_flex_flow(_screen, LV_FLEX_FLOW_COLUMN);

  CreateTemperature(_screen);
  CreateTimerTargetRow(_screen);

  CreateBottomRow(_screen);

  return _screen;
}
void DryerScreen::Loop() {}

}  // namespace toothless
