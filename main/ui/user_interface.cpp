#include "user_interface.hpp"

#include "funlog.h"
#include "ui/chart_history.hpp"
#include "ui/display/display.hpp"
#include "ui/screens/home_screen.hpp"
#include "ui/screens/running_screen.hpp"
#include "ui/screens/settings_screen.hpp"
#include <driver/gpio.h>
#include <esp_check.h>
#include <esp_err.h>
#include <lvgl.h>

#include "helpers.hpp"

namespace toothless {

UserInterface::UserInterface() {
  _display = nullptr;
  _current_screen_state = ScreenState::kRunningScreen;
  _current_screen_obj = nullptr; // Initialize before calling SwitchTo
  _current_screen = nullptr;

  // TODO: make topic strings configurations
  // _chart_history = ChartHistory();
  _chart_history.New("sensor.temperature.chamber");
  _chart_history.New("heater.target.temperature", true);
}

UserInterface::~UserInterface() {
  // TODO: figure out what all needs unique/shared ptrs
  if (_current_screen_obj) {
    lv_obj_delete_async(_current_screen_obj);
    _current_screen_obj = nullptr;
  }
  if (_subscription) {
    ps_free_subscriber(_subscription);
    _subscription = nullptr;
  }
}

esp_err_t UserInterface::Init() {
  _subscription = ps_new_subscriber(10, PS_STRLIST("ui.action"));

  ESP_RETURN_ON_ERROR(Display::Init(), FLOG_SHORT_FILENAME, "Display Initialization failed");

  _display = Display::GetDisplayPtr();

  SwitchTo(_current_screen_state);

  return ESP_OK;
}

void UserInterface::Loop() {
  // value member — safe
  _chart_history.Loop();
  ESP_ERROR_CHECK(HandleSubscriptions());
  if (_current_screen) {
    _current_screen->Loop(); // Update current screen
  }
  return;
}

void UserInterface::BackLight(bool state) {
  FLOG_INFO("Turn %s LCD backlight", state ? "on" : "off");
  gpio_set_level((gpio_num_t)CONFIG_TL_DISPLAY_BACKLIGHT_PIN, state);
}

esp_err_t UserInterface::SwitchTo(ScreenState screen) {
  // If we're already switching, ignore the request
  if (_switching_screen_state) {
    return ESP_OK;
  }
  lv_lock();
  // Store the target state and defer the actual switch
  _switching_screen_state = true;
  std::unique_ptr<Screen> new_screen;

  switch (screen) {
  case ScreenState::kHomeScreen:
    new_screen = std::make_unique<HomeScreen>();
    FLOG_DEBUG("Switching to HOME Screen");
    break;
  case ScreenState::kRunningScreen:
    new_screen = std::make_unique<RunningScreen>(&_chart_history);
    FLOG_DEBUG("Switching to RUNNING Screen");
    break;
  case ScreenState::kSettingsScreen:
    new_screen = std::make_unique<SettingsScreen>();
    FLOG_DEBUG("Switching to SETTINGS Screen");
    break;
  default:
    FLOG_ERROR("ScreenState %d not implemented", screen);
    _switching_screen_state = false; // ← Also reset flag
    lv_unlock();
    return ESP_ERR_NOT_SUPPORTED;
  };

  if (new_screen) {
    // Create the LVGL object
    lv_obj_t *new_screen_obj = new_screen->Create();
    // Switch to new screen (LVGL handles old screen cleanup)
    // lv_screen_load(new_screen_obj);
    lv_screen_load_anim(new_screen_obj, LV_SCREEN_LOAD_ANIM_FADE_IN, 250, 0, true);
    // Now safely replace the old with new
    _current_screen = std::move(new_screen); // Old screen auto-destructs here
    _current_screen_obj = new_screen_obj;
  }

  _switching_screen_state = false;
  lv_unlock();
  return ESP_OK;
}

esp_err_t UserInterface::HandleSubscriptions() {
  ps_msg_t *msg = nullptr;
  for ((msg = ps_get(_subscription, 0)); msg != NULL; (msg = ps_get(_subscription, 0))) {
    FLOG_DEBUG("MSG TOPIC: %s", msg->topic);
    if (ps_has_topic(msg, "ui.action.start") || ps_has_topic(msg, "ui.action.running")) {
      SwitchTo(ScreenState::kRunningScreen);
    } else if (ps_has_topic(msg, "ui.action.stop")) {
      SwitchTo(ScreenState::kHomeScreen);
    } else if (ps_has_topic(msg, "ui.action.settings")) {
      SwitchTo(ScreenState::kSettingsScreen);
    } else {
      FLOG_ERROR("Unhandled topic: %s", msg->topic);
    }
    ps_unref_msg(msg);
  }
  return ESP_OK;
}

} // namespace toothless