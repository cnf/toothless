#include "ui/screens/settings_screen.hpp"

#include <esp_app_desc.h>

#include <cmath>
#include <format>
#include <memory>

#include "config.h"
#include "funlog.h"
#include "ui/display/display.hpp"
#include "ui/screens/screen_helpers.hpp"

extern "C" {
#include <pubsub.h>
}

namespace toothless {

SettingsScreen::SettingsScreen() { _labels = std::make_unique<SettingsScreenLabels>(); }

SettingsScreen::~SettingsScreen() {}

lv_obj_t* SettingsScreen::Create() {
  // esp_log_level_set(FLOG_SHORT_FILENAME, ESP_LOG_DEBUG);
  FLOG_INFO("Creating 480x320 Settings Screen");
  _screen = lv_obj_create(NULL);
  lv_obj_set_style_bg_color(_screen, lv_color_black(), 0);

  _labels->sidebar = true;
  lv_obj_set_style_pad_all(_screen, 10, 0);

  // Vertical flex layout
  lv_obj_set_layout(_screen, LV_LAYOUT_FLEX);
  lv_obj_set_flex_flow(_screen, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_style_pad_gap(_screen, 10, 0);

  // CreateTitle();
  // CreateSettingsList();
  BaseCreate();
  // CreateBackButton();
  SetSidebar(true);

  return _screen;
}

void SettingsScreen::Loop() {}

esp_err_t SettingsScreen::CreateTitle() {
  _labels->title = lv_label_create(_screen);
  lv_label_set_text(_labels->title, "Settings");
  lv_obj_set_style_text_font(_labels->title, &lv_font_montserrat_20, 0);
  lv_obj_set_width(_labels->title, lv_pct(100));
  lv_obj_set_style_text_align(_labels->title, LV_TEXT_ALIGN_CENTER, 0);
  return ESP_OK;
}

void SettingsScreen::MenuBackEventHandler(lv_event_t* e) {
  lv_obj_t* obj = (lv_obj_t*)lv_event_get_target(e);
  // lv_obj_t* menu = (lv_obj_t*)lv_event_get_user_data(e);
  SettingsScreen* screen = (SettingsScreen*)lv_event_get_user_data(e);

  if (lv_menu_back_button_is_root(screen->_labels->menu, obj)) {
    BackButtonHandler(e);
    //   lv_obj_t* mbox1 = lv_msgbox_create(NULL);
    //   lv_msgbox_add_title(mbox1, "Hello");
    //   lv_msgbox_add_text(mbox1, "Root back btn click.");
    //   lv_msgbox_add_close_button(mbox1);
  }
}

esp_err_t SettingsScreen::BaseCreate() {
  FLOG_DEBUG("Creating Settings Menu");
  _labels->menu = lv_menu_create(_screen);
  lv_obj_set_size(_labels->menu, lv_pct(100), 0);
  lv_obj_set_style_pad_gap(_labels->menu, 10, 0);
  lv_obj_set_flex_grow(_labels->menu, 1);
  {
    lv_color_t bg_color = lv_obj_get_style_bg_color(_labels->menu, LV_PART_MAIN);
    if (lv_color_brightness(bg_color) > 127) {
      lv_obj_set_style_bg_color(_labels->menu,
                                lv_color_darken(lv_obj_get_style_bg_color(_labels->menu, LV_PART_MAIN), 10), 0);
    } else {
      lv_obj_set_style_bg_color(_labels->menu,
                                lv_color_darken(lv_obj_get_style_bg_color(_labels->menu, LV_PART_MAIN), 50), 0);
    }
  }

  // Back button
  lv_menu_set_mode_root_back_button(_labels->menu, LV_MENU_ROOT_BACK_BUTTON_ENABLED);
  lv_obj_add_event_cb(_labels->menu, MenuBackEventHandler, LV_EVENT_CLICKED, this);
  lv_obj_t* back_btn = lv_menu_get_main_header_back_button(_labels->menu);
  lv_obj_set_ext_click_area(back_btn, lv_pct(33));
  lv_obj_set_width(back_btn, lv_pct(10));

  /*Create a root page*/
  _labels->root_page = lv_menu_page_create(_labels->menu, NULL);  //"Settings");
  // lv_obj_set_style_pad_hor(_labels->root_page, lv_obj_get_style_pad_left(lv_menu_get_main_header(menu), 0), 0);
  lv_obj_t* section = lv_menu_section_create(_labels->root_page);

  CreateSubMode(_labels->menu, section);
  CreateSubDisplay(_labels->menu, section);

  CreateText(_labels->root_page, NULL, "Info", LV_MENU_ITEM_BUILDER_VARIANT_1);
  section = lv_menu_section_create(_labels->root_page);
  CreateSubFirmwareInfo(_labels->menu, section);
  lv_obj_t* sidebar_switch = CreateSwitch(section, LV_SYMBOL_SETTINGS, NULL, _labels->sidebar);
  lv_obj_add_event_cb(sidebar_switch, SidebarHandler, LV_EVENT_VALUE_CHANGED, this);

  // SetSidebar(_labels->sidebar);

  lv_menu_set_page(_labels->menu, _labels->root_page);

  FLOG_DEBUG("Menu Created");
  return ESP_OK;
}

lv_obj_t* SettingsScreen::CreateText(lv_obj_t* parent, const char* icon, const char* txt,
                                     lv_menu_builder_variant_t builder_variant) {
  return CreateText(parent, icon, txt, NULL, builder_variant);
}

lv_obj_t* SettingsScreen::CreateText(lv_obj_t* parent, const char* icon, const char* txt, const char* fmt,
                                     lv_menu_builder_variant_t builder_variant) {
  lv_obj_t* obj = lv_menu_cont_create(parent);

  lv_obj_t* img = NULL;
  lv_obj_t* label = NULL;

  if (icon) {
    img = lv_image_create(obj);
    lv_image_set_src(img, icon);
  }

  if (txt) {
    label = lv_label_create(obj);
    lv_label_set_text_fmt(label, txt, fmt);
    lv_label_set_long_mode(label, LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_obj_set_flex_grow(label, 1);
  }

  if (builder_variant == LV_MENU_ITEM_BUILDER_VARIANT_2 && icon && txt) {
    lv_obj_add_flag(img, LV_OBJ_FLAG_FLEX_IN_NEW_TRACK);
    lv_obj_swap(img, label);
  }

  return obj;
}

lv_obj_t* SettingsScreen::CreateSwitch(lv_obj_t* parent, const char* icon, const char* txt, bool chk) {
  lv_obj_t* obj = CreateText(parent, icon, txt, LV_MENU_ITEM_BUILDER_VARIANT_1);

  lv_obj_t* sw = lv_switch_create(obj);
  lv_obj_add_state(sw, chk ? LV_STATE_CHECKED : LV_STATE_DEFAULT);

  return sw;
}

lv_obj_t* SettingsScreen::CreateSlider(lv_obj_t* parent, const char* icon, const char* txt, int32_t min, int32_t max,
                                       int32_t val) {
  lv_obj_t* obj = CreateText(parent, icon, txt, LV_MENU_ITEM_BUILDER_VARIANT_2);

  lv_obj_t* slider = lv_slider_create(obj);
  lv_obj_set_flex_grow(slider, 1);
  lv_slider_set_range(slider, min, max);
  lv_slider_set_value(slider, val, LV_ANIM_OFF);

  if (icon == NULL) {
    lv_obj_add_flag(slider, LV_OBJ_FLAG_FLEX_IN_NEW_TRACK);
  }

  return obj;
}

lv_obj_t* SettingsScreen::CreateDropdown(lv_obj_t* parent, const char* icon, const char* txt, const char* options) {
  lv_obj_t* obj = CreateText(parent, icon, txt, LV_MENU_ITEM_BUILDER_VARIANT_1);

  lv_obj_t* dd = lv_dropdown_create(obj);
  lv_dropdown_set_options(dd, options);
  lv_obj_set_width(dd, lv_pct(50));

  return obj;
}

lv_obj_t* SettingsScreen::CreateSubMode(lv_obj_t* parent, lv_obj_t* section) {
  lv_obj_t* sub_mode_page = lv_menu_page_create(_labels->menu, "Mode Configuration");
  // lv_obj_set_style_pad_hor(sub_mode_page, lv_obj_get_style_pad_left(lv_menu_get_main_header(menu), 0), 0);
  lv_menu_separator_create(sub_mode_page);
  lv_obj_t* sub_section = lv_menu_section_create(sub_mode_page);
  CreateDropdown(sub_section, NULL, "Default",
                 "Dryer\n"
                 "Profile\n"
                 "Heater");

  lv_menu_separator_create(sub_mode_page);
  CreateText(sub_section, LV_SYMBOL_LIST, "Dryer Defaults", LV_MENU_ITEM_BUILDER_VARIANT_1);
  CreateText(sub_section, NULL, "Timer", LV_MENU_ITEM_BUILDER_VARIANT_1);
  CreateText(sub_section, NULL, "Temperature", LV_MENU_ITEM_BUILDER_VARIANT_1);

  lv_obj_t* cont = CreateText(section, LV_SYMBOL_SETTINGS, "Mode", LV_MENU_ITEM_BUILDER_VARIANT_1);
  lv_menu_set_load_page_event(_labels->menu, cont, sub_mode_page);
  return sub_mode_page;
};

lv_obj_t* SettingsScreen::CreateSubDisplay(lv_obj_t* parent, lv_obj_t* section) {
  lv_obj_t* sub_display_page = lv_menu_page_create(_labels->menu, "Display Settings");
  // lv_obj_set_style_pad_hor(sub_display_page, lv_obj_get_style_pad_left(lv_menu_get_main_header(menu), 0), 0);
  lv_menu_separator_create(sub_display_page);
  lv_obj_t* sub_section = lv_menu_section_create(sub_display_page);
  CreateSwitch(sub_section, LV_SYMBOL_TINT, "Dark", true);
  CreateSwitch(sub_section, LV_SYMBOL_IMAGE, "Portrait", false);

  lv_obj_t* cont = CreateText(section, LV_SYMBOL_SETTINGS, "Display", LV_MENU_ITEM_BUILDER_VARIANT_1);
  lv_menu_set_load_page_event(_labels->menu, cont, sub_display_page);
  return sub_display_page;
};

lv_obj_t* SettingsScreen::CreateSubFirmwareInfo(lv_obj_t* parent, lv_obj_t* section) {
  const esp_app_desc_t* desc = esp_app_get_description();
  std::string version_info = "Firmware " + std::string(desc->project_name) + "Version " + std::string(desc->version) +
                             ", ESP-IDF Version: " + std::string(desc->idf_ver);
  // ESP_ERROR_CHECK(esp_ota_get_partition_description(esp_ota_get_running_partition(), &desc));
  // ESP_ERR_CHECK(esp_ota_get_running_partition);
  // setenv(ENV_HARDWARE, desc.project_name, 1);
  // setenv(ENV_VERSION, desc.version, 1);
  lv_obj_t* sub_software_info_page = lv_menu_page_create(parent, "Firmware");
  lv_obj_set_style_pad_hor(sub_software_info_page,
                           lv_obj_get_style_pad_left(lv_menu_get_main_header(parent), LV_PART_MAIN), 0);
  lv_obj_t* fsection = lv_menu_section_create(sub_software_info_page);
  CreateText(fsection, LV_SYMBOL_BULLET, "Firmware: %s", desc->project_name, LV_MENU_ITEM_BUILDER_VARIANT_1);
  CreateText(fsection, LV_SYMBOL_BULLET, "Version: %s", desc->version, LV_MENU_ITEM_BUILDER_VARIANT_1);
  CreateText(fsection, LV_SYMBOL_BULLET, "ESP-IDF: %s", desc->idf_ver, LV_MENU_ITEM_BUILDER_VARIANT_1);
  CreateText(fsection, LV_SYMBOL_BULLET, "Build Date: " __DATE__, LV_MENU_ITEM_BUILDER_VARIANT_1);
  CreateText(fsection, LV_SYMBOL_HOME, "URL: https://github.com/cnf/Toothless", LV_MENU_ITEM_BUILDER_VARIANT_1);
  CreateCBButton(fsection, "Reboot", false, ResetHandler, nullptr);

  lv_obj_t* cont = CreateText(section, NULL, "About", LV_MENU_ITEM_BUILDER_VARIANT_1);
  lv_menu_set_load_page_event(parent, cont, sub_software_info_page);

  return sub_software_info_page;
}

esp_err_t SettingsScreen::CreateSettingsList() {
  // Container for settings
  lv_obj_t* settings_container = lv_obj_create(_screen);
  lv_obj_set_size(settings_container, lv_pct(100), LV_SIZE_CONTENT);
  lv_obj_set_layout(settings_container, LV_LAYOUT_FLEX);
  lv_obj_set_flex_flow(settings_container, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_style_pad_gap(settings_container, 10, 0);
  lv_obj_set_flex_grow(settings_container, 1);  // Grow to fill space

  // Target Temperature Setting
  lv_obj_t* temp_row = lv_obj_create(settings_container);
  lv_obj_set_size(temp_row, lv_pct(100), LV_SIZE_CONTENT);
  lv_obj_set_layout(temp_row, LV_LAYOUT_FLEX);
  lv_obj_set_flex_flow(temp_row, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(temp_row, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  lv_obj_add_event_cb(temp_row, NumpadOpenHandler, LV_EVENT_CLICKED, this);

  // Label
  lv_obj_t* temp_label = lv_label_create(temp_row);
  lv_label_set_text(temp_label, "Target:");
  lv_obj_set_style_text_font(temp_label, &lv_font_montserrat_20, 0);
  lv_obj_set_flex_grow(temp_label, 1);

  // Spinbox for target temperature
  _labels->set_target = lv_spinbox_create(temp_row);
  lv_spinbox_set_range(_labels->set_target, 0, kUIMaxTargetTemperatureC);  // 0-300°C range
  lv_spinbox_set_value(_labels->set_target, 35);                           // Default 35°C
  lv_spinbox_set_digit_format(_labels->set_target, 3, 0);                  // 3 digits, 0 decimal places
  lv_spinbox_set_step(_labels->set_target, 1);                             // Step by 1°C for fine control
  // TODO: make removal of cursor conditional on the presense of encoder input
  lv_obj_remove_style(_labels->set_target, NULL, LV_PART_CURSOR);
  lv_obj_set_width(_labels->set_target, 100);
  lv_obj_add_event_cb(_labels->set_target, SettingChangedHandler, LV_EVENT_VALUE_CHANGED, this);
  lv_obj_add_event_cb(_labels->set_target, NumpadOpenHandler, LV_EVENT_CLICKED, this);
  lv_obj_set_style_text_font(_labels->set_target, &lv_font_montserrat_20, 0);

  lv_obj_t* temp_units = lv_label_create(temp_row);
  lv_label_set_text(temp_units, "°C");
  lv_obj_set_style_text_font(temp_units, &lv_font_montserrat_20, 0);

  return ESP_OK;
}

esp_err_t SettingsScreen::SetSidebar(bool mode) {
  FLOG_DEBUG("Toggling menu mode");
  if (mode) {
    _labels->sidebar = true;
    lv_menu_set_page(_labels->menu, NULL);
    lv_menu_set_sidebar_page(_labels->menu, _labels->root_page);
    lv_obj_send_event(lv_obj_get_child(lv_obj_get_child(lv_menu_get_cur_sidebar_page(_labels->menu), 0), 0),
                      LV_EVENT_CLICKED, NULL);
    // lv_menu_get_sidebar_header_back_button(_labels->menu);
    lv_obj_t* sidebar_header = lv_menu_get_sidebar_header(_labels->menu);
    lv_obj_add_flag(sidebar_header, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(sidebar_header, MenuBackEventHandler, LV_EVENT_CLICKED, this);
    // lv_obj_set_style_pad_left(sidebar_header, 30, 0);  // increases clickable area
    lv_obj_set_ext_click_area(sidebar_header, 10);
    lv_obj_set_style_border_width(sidebar_header, 0, 0);
    // lv_obj_set_width(sidebar_header, lv_pct(10));
    lv_obj_t* sidebar_back = lv_menu_get_sidebar_header_back_button(_labels->menu);
    lv_obj_add_flag(sidebar_back, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_ext_click_area(sidebar_back, 10);  // enlarge hitbox around the label

    lv_obj_add_event_cb(sidebar_back, MenuBackEventHandler, LV_EVENT_CLICKED, this);

  } else {
    _labels->sidebar = false;
    lv_menu_set_sidebar_page(_labels->menu, NULL);
    lv_menu_clear_history(_labels->menu); /* Clear history because we will be showing the root page later */
    lv_menu_set_page(_labels->menu, _labels->root_page);
  }
  return ESP_OK;
}

// esp_err_t SettingsScreen::CreateBackButton() {
//   // Spacer to push button to bottom
//   lv_obj_t* spacer = lv_obj_create(_screen);
//   lv_obj_remove_style_all(spacer);
//   lv_obj_set_style_bg_opa(spacer, LV_OPA_TRANSP, 0);
//   lv_obj_set_flex_grow(spacer, 0);
//   lv_obj_set_height(spacer, 0);

//   // Back button
//   lv_obj_t* back_btn = lv_button_create(_screen);
//   lv_obj_set_size(back_btn, lv_pct(100), lv_pct(20));

//   lv_obj_t* label = lv_label_create(back_btn);
//   lv_label_set_text(label, "Back");
//   lv_obj_center(label);

//   lv_obj_add_event_cb(back_btn, BackButtonHandler, LV_EVENT_CLICKED, this);

//   return ESP_OK;
// }

void SettingsScreen::SidebarHandler(lv_event_t* e) {
  lv_event_code_t code = lv_event_get_code(e);
  SettingsScreen* screen = (SettingsScreen*)lv_event_get_user_data(e);
  if (!screen) return;

  // lv_obj_t* menu = (lv_obj_t*)lv_event_get_user_data(e);
  FLOG_DEBUG("Toggling Sidebar");
  lv_obj_t* obj = (lv_obj_t*)lv_event_get_target(e);
  if (code == LV_EVENT_VALUE_CHANGED) {
    if (lv_obj_has_state(obj, LV_STATE_CHECKED)) {
      screen->SetSidebar(true);
    } else {
      screen->SetSidebar(false);
    }
  }
}

void SettingsScreen::BackButtonHandler(lv_event_t* e) {
  FLOG_INFO("Back POOP button pressed");
  SettingsScreen* obj = (SettingsScreen*)lv_event_get_user_data(e);
  if (!obj) return;
  if (obj->_labels->backdrop) lv_obj_delete(obj->_labels->backdrop);
  PS_PUB_NIL("ui.action.return");
  return;
  // SettingsScreen* obj = (SettingsScreen*)lv_event_get_user_data(e);
  // if (!obj) return;
  // ConfirmationContext ctx{.parent_screen = obj->GetScreen(),
  //                         .backdrop = obj->_labels->backdrop,
  //                         .object = nullptr,
  //                         .title = "Apply changes?",
  //                         .message = "Apply the new target temperature?",
  //                         .confirm_text = "Apply",
  //                         .cancel_text = "Cancel",
  //                         .on_confirm =
  //                             [obj](void*) {
  //                               FLOG_INFO("Applying new target temperature: %d°C", (int)obj->_pending_value);
  //                               // Publish new target temperature
  //                               // PS_PUB_INT("heater.target.temperature.set", obj->_pending_value);
  //                               PS_PUB_NIL("ui.action.return");

  //                               obj->_has_pending = false;
  //                               obj->_pending_value = 0;
  //                               if (obj->_labels->backdrop) lv_obj_delete(obj->_labels->backdrop);
  //                             },
  //                         .on_cancel =
  //                             [obj](void*) {
  //                               PS_PUB_NIL("ui.action.return");
  //                               FLOG_INFO("Cancelled applying new target temperature");
  //                               obj->_has_pending = false;
  //                               obj->_pending_value = 0;
  //                               if (obj->_labels->backdrop) lv_obj_delete(obj->_labels->backdrop);
  //                             }};

  // ConfirmationPopup(ctx);
}

void SettingsScreen::SettingChangedHandler(lv_event_t* e) {
  SettingsScreen* obj = (SettingsScreen*)lv_event_get_user_data(e);
  if (!obj) return;
  if (obj->_suppress_events) return;
  lv_obj_t* spinbox = (lv_obj_t*)lv_event_get_target(e);
  int32_t value = lv_spinbox_get_value(spinbox);

  // if (value >= 0 && value <= 9) {
  //   lv_spinbox_set_digit_format(spinbox, 1, 0);
  // } else if (value >= 10 && value <= 99) {
  //   lv_spinbox_set_digit_format(spinbox, 2, 0);
  // } else if (value >= 100 && value <= 999) {
  //   lv_spinbox_set_digit_format(spinbox, 3, 0);
  // }

  obj->_pending_value = value;
  obj->_has_pending = true;
  FLOG_INFO("Pending target temperature: %d°C", (int)value);
  // sync slider visually while suppressing events
  // obj->_suppress_events = true;
  // // if (obj->_labels->slider)
  // //   lv_slider_set_value(obj->_labels->slider, value, LV_ANIM_OFF);
  // obj->_suppress_events = false;
}

void SettingsScreen::LocalNumpadOpenHandler(lv_event_t* e) {
  SettingsScreen* obj = (SettingsScreen*)lv_event_get_user_data(e);

  NumpadContext ctx{.parent_screen = obj->GetScreen(),
                    .backdrop = nullptr,                         // backdrop
                    .target_spinbox = obj->_labels->set_target,  // spinbox
                    .on_confirm = [obj](std::optional<int32_t> val) {
                      if (val.has_value() && !std::isnan(val.value())) {
                        FLOG_INFO("Value: %li", val.value());
                        PS_PUB_INT("heater.target.temperature.set", val.value() * 100);
                      } else {
                        PS_PUB_NIL("heater.target.temperature.set");
                      }
                      // lv_label_set_text(obj->_labels->set_target, )
                    }};
  NumpadOpen(ctx);
}

}  // namespace toothless
