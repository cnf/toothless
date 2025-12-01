#include "ui/screens/settings_screen.hpp"

#include <esp_app_desc.h>

#include <cmath>
#include <format>
#include <memory>

#include "config.h"
#include "funlog.h"
#include "ui/display/display.hpp"
#include "ui/screens/screen_helpers.hpp"
#include "ui/themes/style_registry.hpp"
#include "ui/themes/widget_factories.hpp"

extern "C" {
#include <pubsub.h>
}

namespace toothless {

SettingsScreen::SettingsScreen() { _labels = std::make_unique<SettingsScreenLabels>(); }

SettingsScreen::~SettingsScreen() {}

lv_obj_t* SettingsScreen::Create() {
  FLOG_INFO("Creating 180x640 Settings Screen");
  // esp_log_level_set(FLOG_SHORT_FILENAME, ESP_LOG_DEBUG);
  _screen = ui::CreateScreen();

  _labels->sidebar = true;
  lv_obj_set_style_pad_all(_screen, 10, 0);

  // Vertical flex layout
  lv_obj_set_layout(_screen, LV_LAYOUT_FLEX);
  lv_obj_set_flex_flow(_screen, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_style_pad_gap(_screen, 10, 0);

  CreateMenu();
  // CreateBackButton();
  SetSidebar(true);

  return _screen;
}

void SettingsScreen::Loop() {}

}  // namespace toothless
