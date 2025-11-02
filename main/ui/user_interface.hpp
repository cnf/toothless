#pragma once

// #include <functional>
#include "ui/screens/screen.hpp"
#include <esp_err.h>
#include <lvgl.h>
#include <map>

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
  // void HandlePubsubMessage(const char *topic, ps_msg_t *msg);
  ScreenState GetCurrentScreenState() const { return _current_screen_state; }

private:
  lv_display_t *_display;
  ScreenState _current_screen_state;
  lv_obj_t *_current_screen;
  Screen *_current_screen_obj;
  esp_err_t HandleSubscriptions();
};
} // namespace toothless