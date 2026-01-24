#pragma once

#include <esp_err.h>
#include <lvgl.h>

#include <map>
#include <memory>
#include <string>

#include "config_mgr.hpp"
#include "ui/screens/screen.hpp"

namespace toothless {

// enum { LV_MENU_ITEM_BUILDER_VARIANT_1, LV_MENU_ITEM_BUILDER_VARIANT_2 };
// typedef uint8_t lv_menu_builder_variant_t;

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

  /// @brief Create the settings screen UI
  /// @return LVGL object representing the screen
  lv_obj_t* Create() override;

  /// @brief Main loop for the settings screen
  void Loop() override;

  /// @brief Get screen labels
  /// @return Pointer to screen labels
  ScreenLabels* GetLabels() override { return _labels.get(); };

 private:
  struct WidgetData {
    std::string namespace_name;  //<! Configuration namespace
    std::string key;             //<! Configuration key
    ConfigValueTypes type;       //<! Configuration value type
    Validator validator;         //<! Validator for the setting
  };
  // Small struct just for the tap context
  struct TapEditData {
    std::string namespace_name;
    std::string key;
    std::string format;
    std::string unit;
    ConfigValueTypes type;
  };

  std::unique_ptr<SettingsScreenLabels> _labels;
  std::shared_ptr<Subjects> _subjects;
  std::map<lv_obj_t*, std::shared_ptr<WidgetData>> _widget_map;

  /// @brief Create the main menu structure
  /// @return ESP_OK on success, error code otherwise
  esp_err_t BaseCreate();

  /// @brief Enable or disable the sidebar
  /// @param mode True to enable sidebar, false to disable
  /// @return ESP_OK on success, error code otherwise
  esp_err_t SetSidebar(bool mode);

  /// @brief Build settings UI for a given namespace
  /// @param parent Parent LVGL object
  /// @param namespace_name Configuration namespace
  /// @param title Title for the settings page
  void BuildSettingsUI(lv_obj_t* parent, const char* namespace_name, const char* title);

  /// @brief Build settings UI from given config entries and current values
  /// @param parent Parent LVGL object
  /// @param namespace_name Configuration namespace
  /// @param entries Configuration entries to build UI from
  /// @param current_values Current settings values
  /// @param title Title for the settings page
  void BuildFromEntries(lv_obj_t* parent, const char* namespace_name, const ConfigEntries& entries,
                        const SettingsMap& current_values, std::string title = "");

  /// @brief Build sensor settings UI
  /// @param parent Parent LVGL object
  void BuildSensorSettingsUI(lv_obj_t* parent);

  /// @brief Create Firmware Info subpage
  /// @param parent Parent LVGL object
  /// @param section Section LVGL object
  /// @return LVGL object representing the subpage
  lv_obj_t* CreateSubFirmwareInfo(lv_obj_t* parent, lv_obj_t* section);

  /// @brief Create System Info subpage
  /// @param parent Parent LVGL object
  /// @param root Root LVGL object
  /// @return LVGL object representing the subpage
  lv_obj_t* CreateSubSystemInfo(lv_obj_t* parent, lv_obj_t* root);

  /// @brief Create Status subpage
  /// @param parent Parent LVGL object
  /// @param section Section LVGL object
  /// @return LVGL object representing the subpage
  lv_obj_t* CreateSubStatus(lv_obj_t* parent, lv_obj_t* section);

  /// @defgroup type_widget_builders
  /// @name Setting Type Widget Builders
  /// @brief Functions to build widgets for different setting types
  /// @param parent Parent LVGL object
  /// @param entry Configuration entry
  /// @param current_value Current value of the setting
  /// @{

  lv_obj_t* BuildBoolSetting(lv_obj_t* parent, const ConfigEntry& entry,
                             bool current_value);  //<! Build boolean setting widget
  lv_obj_t* BuildIntSetting(lv_obj_t* parent, const ConfigEntry& entry,
                            int current_value);  //<! Build integer setting widget
  lv_obj_t* BuildDoubleSetting(lv_obj_t* parent, const char* ns, const ConfigEntry& entry,
                               double current_value);  //<! Build double setting widget
  lv_obj_t* BuildStringSetting(lv_obj_t* parent, const ConfigEntry& entry,
                               const std::string& current_value);  //<! Build string setting widget
  lv_obj_t* BuildEnumSetting(lv_obj_t* parent, const ConfigEntry& entry,
                             const std::string& current_value);  //<! Build enum setting widget

  /// @}

  static void OnValueTapped(lv_event_t* e);

  /// @defgroup type_change_handlers Data Type Event Handlers
  /// @brief Event handlers for different data types
  /// @implements @ref lv_event_cb_t
  /// @param e LVGL event object
  /// @{
  static void OnSwitchChanged(lv_event_t* e);
  static void OnSpinboxChanged(lv_event_t* e);
  static void OnSliderChanged(lv_event_t* e);
  static void OnRollerChanged(lv_event_t* e);
  static void OnDropdownChanged(lv_event_t* e);
  static void OnTextareaChanged(lv_event_t* e);
  static void OnWidgetDeleted(lv_event_t* e);
  /// @}

  /// @defgroup General Event Handlers
  /// @brief General event handlers for the settings screen
  /// @implements @ref lv_event_cb_t
  /// @param e LVGL event object
  /// @{
  static void StageEditDoubleHandler(lv_event_t* e);
  static void SidebarHandler(lv_event_t* e);  //<! Handle sidebar toggle events
  static void SidebarObserverCallback(lv_observer_t* observer, lv_subject_t* subject);
  static void MenuBackEventHandler(lv_event_t* e);  //<! Handle back button events
  static void BackButtonHandler(lv_event_t* e);     //<! Handle back button events
  static void ResetHandler(lv_event_t* e);
  /// @}

  // Helpers

  /// @brief Parse enum values from format string
  /// @param format Format string
  /// @return Vector of enum values
  std::vector<std::string> ParseEnumFromFormat(const std::string& format);

  /// @brief Parse validator from format string
  /// @note FIXME: This is duplicated from config_mgr.hpp, should be refactored
  /// @param format Format string
  /// @return Validator struct
  static Validator ParseValidatorFromFormat(const std::string& format);
};

}  // namespace toothless
