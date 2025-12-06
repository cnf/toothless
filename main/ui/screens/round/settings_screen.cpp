#include "ui/screens/settings_screen.hpp"

#include <cmath>
#include <format>
#include <memory>

#include "config.h"
#include "funlog.h"
#include "ui/display/display.hpp"
#include "ui/screens/screen_helpers.hpp"
#include "ui/themes/widget_factories.hpp"

extern "C" {
#include <pubsub.h>
}

namespace toothless {

SettingsScreen::SettingsScreen() { _labels = std::make_unique<SettingsScreenLabels>(); }

SettingsScreen::~SettingsScreen() {}

lv_obj_t* SettingsScreen::Create() {
  // esp_log_level_set(FLOG_SHORT_FILENAME, ESP_LOG_DEBUG);
  FLOG_INFO("Creating rectangular Settings Screen");
  _screen = ui::CreateScreen();

  if (lv_display_get_horizontal_resolution(NULL) >= 400) {
    _labels->sidebar = true;
  } else {
    _labels->sidebar = false;
  }

  // Vertical flex layout
  // lv_obj_set_layout(_screen, LV_LAYOUT_FLEX);
  // lv_obj_set_flex_flow(_screen, LV_FLEX_FLOW_COLUMN);
  // lv_obj_set_style_pad_gap(_screen, 10, 0);

  BaseCreate();
  // CreateBackButton();
  SetSidebar(_labels->sidebar);

  return _screen;
}

void SettingsScreen::Loop() {}

}  // namespace toothless
