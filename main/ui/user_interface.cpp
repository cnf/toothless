#include "user_interface.hpp"

#include "funlog.h"
#include "ui/display/display.hpp"
#include "ui/screens/home_screen.hpp"
#include "ui/screens/running_screen.hpp"
#include <esp_check.h>
#include <lvgl.h>

#include "helpers.hpp"

namespace toothless {

UserInterface::UserInterface() {
  _display = nullptr;
  _current_screen_state = ScreenState::HOME;
  _current_screen_obj = nullptr; // Initialize before calling SwitchTo
  _current_screen = nullptr;
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
  ESP_ERROR_CHECK(HandleSubscriptions());
  if (_current_screen) {
    _current_screen->Loop(); // Update current screen
  }
  return;
}

esp_err_t UserInterface::SwitchTo(ScreenState screen) {
  MemPressure();
  FLOG_INFO("SwitchTo: %d", (int)screen);
  // If we're already switching, ignore the request
  if (_switching_screen_state) {
    return ESP_OK;
  }
  // Store the target state and defer the actual switch
  _switching_screen_state = true;
  std::unique_ptr<Screen> new_screen;

  switch (screen) {
  case ScreenState::HOME:
    new_screen = std::make_unique<HomeScreen>();
    break;
  case ScreenState::RUNNING:
    new_screen = std::make_unique<RunningScreen>();
    break;
  default:
    FLOG_ERROR("ScreenState %d not implemented", screen);
    _switching_screen_state = false; // ← Also reset flag
    return ESP_ERR_NOT_SUPPORTED;
  };

  if (new_screen) {
    // Create the LVGL object
    lv_obj_t *new_screen_obj = new_screen->Create();
    // Switch to new screen (LVGL handles old screen cleanup)
    lv_screen_load(new_screen_obj);
    // Now safely replace the old with new
    _current_screen = std::move(new_screen); // Old screen auto-destructs here
    _current_screen_obj = new_screen_obj;
  }

  MemPressure();

  _switching_screen_state = false;
  return ESP_OK;
}

esp_err_t UserInterface::HandleSubscriptions() {
  ps_msg_t *msg = nullptr;
  for ((msg = ps_get(_subscription, 0)); msg != NULL; (msg = ps_get(_subscription, 0))) {
    FLOG_INFO("Got message...");
    if (ps_has_topic(msg, "ui.action.start")) {
      FLOG_INFO("START!!!");
      SwitchTo(ScreenState::RUNNING);
    } else if (ps_has_topic(msg, "ui.action.stop")) {
      FLOG_INFO("STOP!!!");
      SwitchTo(ScreenState::HOME);
    } else {
      FLOG_INFO("Unhandled topic: %s", msg->topic);
    }
    ps_unref_msg(msg);
  }
  return ESP_OK;
}

} // namespace toothless