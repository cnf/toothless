#pragma once

#include <lvgl.h>

#include <functional>
#include <optional>
#include <string>

#include "impl_config.hpp"
// #include "ui/display/display.hpp"

namespace toothless {

#if CONFIG_IMPL_DISPLAY_HRES >= 400
constexpr bool kSidebarEnabledByDefault = true;
#else
constexpr bool kSidebarEnabledByDefault = false;
#endif

#if CONFIG_IMPL_DISPLAY_VRES >= 300
constexpr bool kTallDisplay = true;
#else
constexpr bool kTallDisplay = false;

#endif

struct NumpadContext {
  lv_obj_t* parent_screen = nullptr;
  std::function<void(std::optional<double>)> on_confirm;
  std::optional<double> initial_value = std::nullopt;
  int decimal_places = 0;  // 0 = int mode, >0 = double mode
  std::optional<double> min = std::nullopt;
  std::optional<double> max = std::nullopt;
};

struct NumpadState {
  lv_obj_t* numpad;
  lv_obj_t* numpadtextarea;
  lv_obj_t* backdrop;
  int decimal_places = 0;
  std::function<void(std::optional<double>)> on_confirm;
};

// struct NumpadContext {
//   lv_obj_t* parent_screen = nullptr;                       // Screen where numpad is opened
//   lv_obj_t* backdrop = nullptr;                            // Where to create backdrop
//   lv_obj_t* target_spinbox = nullptr;                      // Spinbox to update
//   std::function<void(std::optional<int32_t>)> on_confirm;  // Callback when user confirms
//   std::optional<int32_t> initial_value = std::nullopt;     // Initial value to show
// };

// struct NumpadDoubleContext {
//   lv_obj_t* parent_screen = nullptr;                      // Screen where numpad is opened
//   lv_obj_t* backdrop = nullptr;                           // Where to create backdrop
//   lv_obj_t* target_spinbox = nullptr;                     // Spinbox to update
//   std::function<void(std::optional<double>)> on_confirm;  // Callback when user confirms
//   std::optional<double> initial_value = std::nullopt;     // Initial value to show
//   std::optional<int> decimal_places = std::nullopt;       // Number of decimal places to allow
//   std::optional<double> step = std::nullopt;              // Step value for increment/decrement
//   std::optional<double> min = std::nullopt;               //
//   std::optional<double> max = std::nullopt;               //
// };

// struct NumpadState {
//   lv_obj_t* numpad;  // Numpad object
//   lv_obj_t* numpadtextarea;
//   lv_obj_t* ok_btn;
//   lv_obj_t* backdrop;                                     // Backdrop object
//   lv_obj_t* target_spinbox;                               // Spinbox being edited
//   std::function<void(std::optional<double>)> on_confirm;  // Callback when user confirms
// };

struct NumberRollerContext {
  lv_obj_t* parent_screen = nullptr;                       // Screen where roller is opened
  lv_obj_t* backdrop = nullptr;                            // Where to create backdrop
  lv_obj_t* target_spinbox = nullptr;                      // Spinbox to update
  std::function<void(std::optional<int32_t>)> on_confirm;  // Callback when user confirms
};

struct NumberRollerState {
  lv_obj_t* col_a;  // Roller object
  lv_obj_t* col_b;
  lv_obj_t* col_c;
  lv_obj_t* ok_btn;
  lv_obj_t* backdrop;                                      // Backdrop object
  lv_obj_t* target_spinbox;                                // Spinbox being edited
  std::function<void(std::optional<int32_t>)> on_confirm;  // Callback when user confirms
};

struct TextAreaOverlayState {
  lv_obj_t* backdrop;
  lv_obj_t* original_textarea;  // The textarea that triggered the overlay
  lv_obj_t* holder;             // Copy of textarea inside overlay
};

struct DoubleOverlayState {
  lv_obj_t* backdrop;
  lv_obj_t* original_obj;  // The spinbox that triggered the overlay
  lv_obj_t* holder;        // Copy of spinbox inside overlay
};

struct ConfirmationContext {
  lv_obj_t* parent_screen = nullptr;  // Screen where confirmation is opened
  lv_obj_t* backdrop = nullptr;       // Where to create backdrop
  lv_obj_t* object;                   // object to act on
  std::string title;
  std::string message;
  std::string confirm_text;
  std::string cancel_text;
  std::function<void(void*)> on_confirm;  // Callback when user confirms
  std::function<void(void*)> on_cancel;   // Callback when user cancels
};

struct ConfirmationState {
  lv_obj_t* msgbox;    // Message box object
  lv_obj_t* backdrop;  // Backdrop object
  lv_obj_t* object;    // object to act on
  std::string title;
  std::string message;
  std::string confirm_text;
  std::string cancel_text;
  std::function<void(void*)> on_confirm;  // Callback when user confirms
  std::function<void(void*)> on_cancel;   // Callback when user cancels
};

struct ModeSwitcherState {
  lv_obj_t* backdrop;
};

// lv_obj_t* CreateBackdrop(lv_obj_t* screen);

// static void BackdropDeleteCb(lv_event_t* e);

lv_obj_t* MainChart(lv_obj_t* parent, size_t max_points);

void NumpadOpen(const NumpadContext& context);

void NumpadKeyHandler(lv_event_t* e);

void NumPadCleanupHandler(lv_event_t* e);

void TimeRollerOpen(const NumberRollerContext& ctx);

void TimeRollerHandler(lv_event_t* e);

void TimeRollerCleanupHandler(lv_event_t* e);

void TextAreaEventHandler(lv_event_t* e);

void TextAreaFullscreenEventHandler(lv_event_t* e);

void ConfirmationPopup(const ConfirmationContext& ctx);

void ConfirmationHandler(lv_event_t* e);

lv_obj_t* CreateModeSwitcher(lv_obj_t* screen);

void ModeSwitcherHandler(lv_event_t* e);

lv_obj_t* CreateMidSection(lv_obj_t* parent);

void TimerHandler(lv_event_t* e);

lv_obj_t* CreateBottomRow(lv_obj_t* container);

lv_obj_t* CreateStartStopButton(lv_obj_t* container);

lv_obj_t* CreateModeButton(lv_obj_t* container);

lv_obj_t* CreateSettingsButton(lv_obj_t* container);

lv_obj_t* CreateCBButton(lv_obj_t* parent, const char* txt, bool grow, lv_event_cb_t callback, void* user_data);

void ButtonEventHandler(lv_event_t* e);

void CreateStopConfirmation(lv_obj_t* e);

void StopConfirmationHandler(lv_event_t* e);

// void TakeSnapshot();

void AutoDeleter(lv_obj_t* target);

}  // namespace toothless