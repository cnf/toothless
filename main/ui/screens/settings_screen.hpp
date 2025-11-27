#pragma once

#include <esp_err.h>
#include <lvgl.h>

#include <map>
#include <memory>
#include <string>

#include "config_mgr.hpp"
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
  bool sidebar;
  lv_obj_t* root_page;
  lv_obj_t* section;
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
  // void BuildSettingsUI(lv_obj_t* parent, const char* namespace_name);
  void BuildSettingsUI(lv_obj_t* parent, const char* namespace_name, const char* title);

  void BuildFromEntries(lv_obj_t* parent, const char* namespace_name, const ConfigEntries& entries,
                        const SettingsMap& current_values, std::string title = "");

 private:
  struct WidgetData {
    std::string namespace_name;
    std::string key;
    ConfigValueTypes type;
  };
  std::unique_ptr<SettingsScreenLabels> _labels;
  std::map<lv_obj_t*, WidgetData*> _widget_map;

  // Widget builders per type
  lv_obj_t* BuildBoolSetting(lv_obj_t* parent, const ConfigEntry& entry, bool current_value);
  lv_obj_t* BuildIntSetting(lv_obj_t* parent, const ConfigEntry& entry, int current_value);
  lv_obj_t* BuildDoubleSetting(lv_obj_t* parent, const ConfigEntry& entry, double current_value);
  lv_obj_t* BuildStringSetting(lv_obj_t* parent, const ConfigEntry& entry, const std::string& current_value);
  lv_obj_t* BuildEnumSetting(lv_obj_t* parent, const ConfigEntry& entry, const std::string& current_value);

  // Helpers
  std::vector<std::string> ParseEnumFromFormat(const std::string& format);
  Validator ParseValidatorFromFormat(const std::string& format);

  // Event handlers
  static void OnSwitchChanged(lv_event_t* e);
  static void OnSliderChanged(lv_event_t* e);
  static void OnRollerChanged(lv_event_t* e);
  static void OnDropdownChanged(lv_event_t* e);

  // UI construction helpers
  esp_err_t GenerateFromConfig();
  esp_err_t CreateTitle();
  esp_err_t LocalCreateTitle();
  static void MenuBackEventHandler(lv_event_t* e);
  esp_err_t LocalCreateMenu();
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
  lv_obj_t* CreateSubSystemInfo(lv_obj_t* parent, lv_obj_t* root);
  static void ResetHandler(lv_event_t* e);
  esp_err_t CreateSettingsList();
  esp_err_t CreateBackButton();

  esp_err_t SetSidebar(bool mode);

  // Event handlers
  static void SidebarHandler(lv_event_t* e);
  static void BackButtonHandler(lv_event_t* e);
  static void SettingChangedHandler(lv_event_t* e);
  static void SliderChangedHandler(lv_event_t* e);
  static void NumpadOpenHandler(lv_event_t* e);
  static void ConfirmApply();
  static void ConfirmApplyHandler(lv_event_t* e);
  static void ConfirmCancelHandler(lv_event_t* e);
};

}  // namespace toothless
