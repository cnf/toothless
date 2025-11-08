#pragma once

#include <lvgl.h>

#include <functional>

#include "ui/screens/screen.hpp"

namespace toothless {

struct NumpadContext {
  lv_obj_t* parent_screen = nullptr;                       // Screen where numpad is opened
  lv_obj_t* backdrop = nullptr;                            // Where to create backdrop
  lv_obj_t* target_spinbox = nullptr;                      // Spinbox to update
  std::function<void(std::optional<int32_t>)> on_confirm;  // Callback when user confirms
};

struct NumpadState {
  lv_obj_t* numpad;  // Numpad object
  lv_obj_t* numpadtextarea;
  lv_obj_t* ok_btn;
  lv_obj_t* backdrop;                                      // Backdrop object
  lv_obj_t* target_spinbox;                                // Spinbox being edited
  std::function<void(std::optional<int32_t>)> on_confirm;  // Callback when user confirms
};

struct ConfirmationContext {
  lv_obj_t* parent_screen = nullptr;  // Screen where confirmation is opened
  lv_obj_t* backdrop = nullptr;       // Where to create backdrop
  std::string title;
  std::string message;
  std::string confirm_text;
  std::string cancel_text;
  std::function<void()> on_confirm;  // Callback when user confirms
  std::function<void()> on_cancel;   // Callback when user cancels
};

struct ConfirmationState {
  lv_obj_t* msgbox;    // Message box object
  lv_obj_t* backdrop;  // Backdrop object
  std::string title;
  std::string message;
  std::string confirm_text;
  std::string cancel_text;
  std::function<void()> on_confirm;  // Callback when user confirms
  std::function<void()> on_cancel;   // Callback when user cancels
};

lv_obj_t* CreateBackdrop(lv_obj_t* screen);

void NumpadOpen(const NumpadContext& context);

void NumpadKeyHandler(lv_event_t* e);

void NumPadCleanupHandler(lv_event_t* e);

void ConfirmationPopup(const ConfirmationContext& ctx);

void ConfirmationHandler(lv_event_t* e);

}  // namespace toothless