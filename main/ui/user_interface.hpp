#pragma once

// #include <functional>
#include <esp_err.h>
#include <lvgl.h>

#include <map>
#include <memory>

#include "config_mgr.hpp"
#include "heater/heater.hpp"
#include "ui/chart_history.hpp"
#include "ui/screens/screen.hpp"
#include "ui/subjects.hpp"
#include "ui/themes/theme_config.hpp"

extern "C" {
#include <pubsub.h>
}

namespace toothless {

struct UserInterfaceConfig {
  uint8_t brightness;
  bool dark_mode = true;
  bool sidebar = true;
  bool portrait = false;
  std::string theme = "toothless";
};

inline ConfigEntries ui_config_entries = {
    ConfigEntry("sidebar", "Toggle sidebar display", "", true, ""),
    ConfigEntry("theme", "UI theme", themes::MakeFormat(), std::string("toothless"), ""),
    ConfigEntry("brightness", "Screen brightness (0-100)", "min=0,max=100", 80, "%"),
    // ConfigEntry("portrait", "Toggle portrait display mode", "", false, ""),
    // ConfigEntry("dark_mode", "Toggle Dark mode theme", "", true, ""),

};

namespace topics::ui {
// static constexpr char kTopicUIAction[] = "ui.action";
const char name[10] = "ui";
}  // namespace topics::ui

enum ScreenList { kErrorScreen, kSettingsScreen, kDryerScreen, kReflowScreen, kProfilesScreen, kAutotuneScreen };

class UserInterface {
 public:
  UserInterface();
  ~UserInterface();

  static esp_err_t Start();

  esp_err_t Init();
  void Loop();
  // void BackLight(bool state);
  esp_err_t SwitchTo(ScreenList screen);
  void PerformSwitchTo();
  // void HandlePubsubMessage(const char *topic, ps_msg_t *msg);
  ScreenList GetCurrentScreenState() const { return _current_screen_state; }

 private:
  ps_subscriber_t* _subscription;           // pubsub subscription
  ConfigEntries* _config_entries;           // UI configuration entries
  std::shared_ptr<SettingsMap> _config;     // UI settings map
  lv_display_t* _display;                   // lvgl display instance
  lv_obj_t* _sysmon;                        // system monitor object
  heater::Mode _mode;                       // current heater mode
  ScreenList _current_screen_state;         // current screen state enum
  lv_obj_t* _current_screen_obj;            // lvvl current screen object
  std::unique_ptr<Screen> _current_screen;  // current screen instance
  bool _switching_screen_state = false;     // switchin state flag
  ChartHistory _chart_history;              // we are owner, no shared_ptr
  std::shared_ptr<Subjects> _subjects;      // UI subjects

  esp_err_t HandleSubscriptions();
  esp_err_t ApplySettings();
};
}  // namespace toothless