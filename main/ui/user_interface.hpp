#pragma once

// #include <functional>
#include "ui/screens/screen.hpp"
#include <esp_err.h>
#include <lvgl.h>
#include <map>
#include <memory>

extern "C" {
#include <pubsub.h>
}

namespace toothless {

enum ScreenState { HOME, SETTINGS, PROFILE, RUNNING, ERROR };

class UserInterface {
public:
  UserInterface();
  ~UserInterface();

  esp_err_t Init();
  void Loop();
  esp_err_t SwitchTo(ScreenState screen);
  void PerformSwitchTo();
  // void HandlePubsubMessage(const char *topic, ps_msg_t *msg);
  ScreenState GetCurrentScreenState() const { return _current_screen_state; }

private:
  ps_subscriber_t *_subscription;
  lv_display_t *_display;
  ScreenState _current_screen_state;
  lv_obj_t *_current_screen_obj;
  std::unique_ptr<Screen> _current_screen;
  // Screen *_current_screen;
  bool _switching_screen_state = false;
  esp_err_t HandleSubscriptions();
};
} // namespace toothless