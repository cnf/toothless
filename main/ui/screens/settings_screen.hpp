#pragma once

#include <esp_err.h>

#include <memory>

#include "ui/screens/screen.hpp"

namespace toothless {

enum { LV_MENU_ITEM_BUILDER_VARIANT_1, LV_MENU_ITEM_BUILDER_VARIANT_2 };
typedef uint8_t lv_menu_builder_variant_t;

struct ModeContext {
  lv_obj_t* menu;
  lv_obj_t* root_page;
};

struct SettingsScreenLabels : public ScreenLabels {
  lv_obj_t* title;
  lv_obj_t* menu;
  bool menu_mode;
  lv_obj_t* root_page;
  // lv_obj_t *backdrop;
  lv_obj_t* set_target = nullptr;
  // lv_obj_t *slider = nullptr;
  lv_obj_t* numpad_btn = nullptr;
  lv_obj_t* textarea = nullptr;
  lv_obj_t* numpad = nullptr;
  lv_obj_t* ok_btn = nullptr;
};

class SettingsScreen : public Screen {
 public:
  SettingsScreen();
  ~SettingsScreen();

  lv_obj_t* Create() override;
  void Loop() override;
  ScreenLabels* GetLabels() override { return _labels.get(); };

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
  static void back_event_handler(lv_event_t* e);
  esp_err_t CreateMenu();

  /// @brief
  /// @param parent lv_obj_t* to enter text under
  /// @param icon Icon to set for the Text entry, or NUL
  /// @param txt Text to show
  /// @param builder_variant idno yet...
  /// @return
  lv_obj_t* CreateText(lv_obj_t* parent, const char* icon, const char* txt, lv_menu_builder_variant_t builder_variant);
  lv_obj_t* CreateText(lv_obj_t* parent, const char* icon, const char* txt, const char* fmt,
                       lv_menu_builder_variant_t builder_variant);
  lv_obj_t* CreateSwitch(lv_obj_t* parent, const char* icon, const char* txt, bool chk);
  lv_obj_t* CreateSlider(lv_obj_t* parent, const char* icon, const char* txt, int32_t min, int32_t max, int32_t val);
  lv_obj_t* CreateDropdown(lv_obj_t* parent, const char* icon, const char* txt, const char* options);

  lv_obj_t* CreateSubMode(lv_obj_t* parent, lv_obj_t* section);
  lv_obj_t* CreateSubDisplay(lv_obj_t* parent, lv_obj_t* section);
  lv_obj_t* CreateSubFirmwareInfo(lv_obj_t* parent, lv_obj_t* section);
  esp_err_t CreateSettingsList();
  esp_err_t CreateBackButton();

  esp_err_t SetMode(bool mode);

  // Event handlers
  static void ModeHandler(lv_event_t* e);
  static void BackButtonHandler(lv_event_t* e);
  static void SettingChangedHandler(lv_event_t* e);
  static void SliderChangedHandler(lv_event_t* e);
  static void NumpadOpenHandler(lv_event_t* e);
  static void ConfirmApply();
  static void ConfirmApplyHandler(lv_event_t* e);
  static void ConfirmCancelHandler(lv_event_t* e);
};

}  // namespace toothless
