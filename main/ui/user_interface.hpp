#pragma once

// #include <functional>
#include <esp_err.h>
#include <lvgl.h>

#include <map>
#include <memory>

#include "heater/heater.hpp"
#include "ui/chart_history.hpp"
#include "ui/screens/screen.hpp"

extern "C" {
#include <pubsub.h>
}

namespace toothless {

enum ScreenList {
  kErrorScreen,
  kSettingsScreen,
  kDryerScreen,
  kReflowScreen,
};

class UserInterface {
 public:
  UserInterface();
  ~UserInterface();

  static esp_err_t Start();

  esp_err_t Init();
  void Loop();
  void BackLight(bool state);
  esp_err_t SwitchTo(ScreenList screen);
  void PerformSwitchTo();
  // void HandlePubsubMessage(const char *topic, ps_msg_t *msg);
  ScreenList GetCurrentScreenState() const { return _current_screen_state; }

 private:
  ps_subscriber_t* _subscription;           // pubsub subscription
  lv_display_t* _display;                   // lvgl display instance
  lv_obj_t* _sysmon;                        // system monitor object
  heater::Mode _mode;                       // current heater mode
  ScreenList _current_screen_state;         // current screen state enum
  lv_obj_t* _current_screen_obj;            // lvvl current screen object
  std::unique_ptr<Screen> _current_screen;  // current screen instance
  bool _switching_screen_state = false;     // switchin state flag
  ChartHistory _chart_history;              // we are owner, no shared_ptr

  esp_err_t HandleSubscriptions();
};
}  // namespace toothless