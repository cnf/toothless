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

lv_obj_t* SettingsScreen::Create() {
  // esp_log_level_set(FLOG_SHORT_FILENAME, ESP_LOG_DEBUG);
  FLOG_INFO("Creating rectangular Settings Screen");
  _screen = ui::CreateScreen();

  if (lv_display_get_horizontal_resolution(NULL) >= 400) {
    _labels->sidebar = true;
  } else {
    _labels->sidebar = false;
  }

  BaseCreate();
  SetSidebar(_labels->sidebar);

  return _screen;
}

void SettingsScreen::Loop() {}

}  // namespace toothless
