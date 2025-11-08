#pragma once

// #include <functional>
#include "ui/chart_history.hpp"
#include "ui/screens/screen.hpp"
#include <esp_err.h>
#include <lvgl.h>
#include <map>
#include <memory>

extern "C" {
#include <pubsub.h>
}

namespace toothless {

enum ScreenState { kHomeScreen, kSettingsScreen, kRunningScreen, kErrorScreen };

class UserInterface {
public:
  UserInterface();
  ~UserInterface();

  esp_err_t Init();
  void Loop();
  void BackLight(bool state);
  esp_err_t SwitchTo(ScreenState screen);
  void PerformSwitchTo();
  // void HandlePubsubMessage(const char *topic, ps_msg_t *msg);
  ScreenState GetCurrentScreenState() const { return _current_screen_state; }

private:
  ps_subscriber_t *_subscription;          // pubsub subscription
  lv_display_t *_display;                  // lvgl display instance
  ScreenState _current_screen_state;       // current screen state enum
  lv_obj_t *_current_screen_obj;           // lvvl current screen object
  std::unique_ptr<Screen> _current_screen; // current screen instance
  bool _switching_screen_state = false;    // switchin state flag
  ChartHistory _chart_history;             // we are owner, no shared_ptr

  esp_err_t HandleSubscriptions();
};
} // namespace toothless