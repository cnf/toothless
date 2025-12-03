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
  lv_obj_t* parent_screen = nullptr;                       // Screen where numpad is opened
  lv_obj_t* backdrop = nullptr;                            // Where to create backdrop
  lv_obj_t* target_spinbox = nullptr;                      // Spinbox to update
  std::function<void(std::optional<int32_t>)> on_confirm;  // Callback when user confirms
  std::optional<int32_t> initial_value = std::nullopt;     // Initial value to show
};

struct NumpadState {
  lv_obj_t* numpad;  // Numpad object
  lv_obj_t* numpadtextarea;
  lv_obj_t* ok_btn;
  lv_obj_t* backdrop;                                      // Backdrop object
  lv_obj_t* target_spinbox;                                // Spinbox being edited
  std::function<void(std::optional<int32_t>)> on_confirm;  // Callback when user confirms
};

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

lv_obj_t* CreateBackdrop(lv_obj_t* screen);

static void BackdropDeleteCb(lv_event_t* e);

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

lv_obj_t* CreateBottomRow(lv_obj_t* container);

lv_obj_t* CreateStartStopButton(lv_obj_t* container);

lv_obj_t* CreateModeButton(lv_obj_t* container);

lv_obj_t* CreateSettingsButton(lv_obj_t* container);

lv_obj_t* CreateText(lv_obj_t* parent, const char* icon, const char* txt, bool builder_variant);

lv_obj_t* CreateText(lv_obj_t* parent, const char* icon, const char* txt, const char* fmt, bool builder_variant);

lv_obj_t* CreateSwitch(lv_obj_t* parent, const char* icon, const char* txt, bool chk);

lv_obj_t* CreateSlider(lv_obj_t* parent, const char* icon, const char* txt, int32_t min, int32_t max, int32_t val);

lv_obj_t* CreateButton(lv_obj_t* parent, const char* txt, bool grow);

lv_obj_t* CreateCBButton(lv_obj_t* parent, const char* txt, bool grow, lv_event_cb_t callback, void* user_data);

void ButtonEventHandler(lv_event_t* e);

void CreateStopConfirmation(lv_obj_t* e);

void StopConfirmationHandler(lv_event_t* e);

}  // namespace toothless