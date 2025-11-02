#include "user_interface.hpp"

#include "funlog.h"
#include "ui/display/display.hpp"
#include "ui/screens/home_screen.hpp"
#include <esp_check.h>
#include <lvgl.h>

namespace toothless {

UserInterface::UserInterface() {
  _display = nullptr;
  _current_screen_state = ScreenState::HOME;
  _current_screen_obj = nullptr; // Initialize before calling SwitchTo
  _current_screen = nullptr;
}

UserInterface::~UserInterface() {
  lv_obj_delete(lv_display_get_screen_active(_display));
  delete _current_screen_obj;
}

esp_err_t UserInterface::Init() {
  ESP_RETURN_ON_ERROR(Display::Init(), FLOG_SHORT_FILENAME, "Display Initialization failed");
  _display = Display::GetDisplayPtr();
  SwitchTo(_current_screen_state);
  return ESP_OK;
}

void UserInterface::Loop() {
  ESP_ERROR_CHECK(HandleSubscriptions());
  if (_current_screen_obj) {
    _current_screen_obj->Loop(); // Update current screen
  }
  return;
}

esp_err_t UserInterface::SwitchTo(ScreenState screen) {
  Screen *new_obj = nullptr;
  switch (screen) {
  case ScreenState::HOME:
    new_obj = new HomeScreen();
    break;
  // case ScreenState::SETTINGS:
  //   new_obj = new SettingsScreen();
  //   break;
  default:
    FLOG_ERROR("ScreenState %d not implemented", screen);
    return ESP_ERR_NOT_SUPPORTED;
  };
  if (new_obj == nullptr) {
    FLOG_ERROR("Failed to create screen %d", screen);
    return ESP_ERR_INVALID_STATE;
  }
  // Clean up current screen if it exists
  if (_current_screen) {
    lv_obj_delete(_current_screen);
  }
  if (_current_screen_obj) {
    delete _current_screen_obj;
  }
  _current_screen_state = screen;
  _current_screen_obj = new_obj;
  _current_screen = _current_screen_obj->Create();
  lv_screen_load(_current_screen);
  return ESP_OK;
}

esp_err_t UserInterface::HandleSubscriptions() { return ESP_OK; }

} // namespace toothless