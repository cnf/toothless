#pragma once

#include "ui/screens/screen.hpp"
#include <esp_err.h>
#include <memory>

namespace toothless {

// static const char *numpad_btn_map[] = {"1", "2", "3", "4",          "5", "6",
//  "7", "8", "9", LV_SYMBOL_OK, "0", LV_SYMBOL_BACKSPACE};

struct SettingsScreenLabels : public ScreenLabels {
  lv_obj_t *title;
  // lv_obj_t *backdrop;
  lv_obj_t *set_target = nullptr;
  // lv_obj_t *slider = nullptr;
  lv_obj_t *numpad_btn = nullptr;
  lv_obj_t *textarea = nullptr;
  lv_obj_t *numpad = nullptr;
  lv_obj_t *ok_btn = nullptr;
};

class SettingsScreen : public Screen {
public:
  SettingsScreen();
  ~SettingsScreen();

  lv_obj_t *Create() override;
  void Loop() override;
  ScreenLabels *GetLabels() override { return _labels.get(); };

private:
  std::unique_ptr<SettingsScreenLabels> _labels;

  // Pending change state (not yet published)
  // bool _has_pending = false;
  // int32_t _pending_value = 0;
  // lv_obj_t *_confirm_msgbox = nullptr;
  // // Event guard to avoid re-entrant LVGL event loops when updating controls
  // bool _suppress_events = false;
  // Track the OK button created with the keyboard so we can delete it

  // UI construction helpers
  esp_err_t CreateTitle();
  esp_err_t CreateSettingsList();
  esp_err_t CreateBackButton();

  // Event handlers
  static void BackButtonHandler(lv_event_t *e);
  static void SettingChangedHandler(lv_event_t *e);
  static void SliderChangedHandler(lv_event_t *e);
  static void NumpadOpenHandler(lv_event_t *e);
  static void ConfirmApply();
  static void ConfirmApplyHandler(lv_event_t *e);
  static void ConfirmCancelHandler(lv_event_t *e);
};

} // namespace toothless
