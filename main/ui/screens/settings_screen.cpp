#include "ui/screens/settings_screen.hpp"

#include <cmath>

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
  _screen = lv_obj_create(NULL);
  lv_obj_set_style_pad_all(_screen, 10, 0);

  // Vertical flex layout
  lv_obj_set_layout(_screen, LV_LAYOUT_FLEX);
  lv_obj_set_flex_flow(_screen, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_style_pad_gap(_screen, 10, 0);

  CreateTitle();
  CreateSettingsList();
  CreateBackButton();

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

esp_err_t SettingsScreen::CreateBackButton() {
  // Spacer to push button to bottom
  lv_obj_t* spacer = lv_obj_create(_screen);
  lv_obj_remove_style_all(spacer);
  lv_obj_set_style_bg_opa(spacer, LV_OPA_TRANSP, 0);
  lv_obj_set_flex_grow(spacer, 1);
  lv_obj_set_height(spacer, 0);

  // Back button
  lv_obj_t* back_btn = lv_button_create(_screen);
  lv_obj_set_size(back_btn, lv_pct(100), lv_pct(20));

  lv_obj_t* label = lv_label_create(back_btn);
  lv_label_set_text(label, "Back");
  lv_obj_center(label);

  lv_obj_add_event_cb(back_btn, BackButtonHandler, LV_EVENT_CLICKED, this);

  return ESP_OK;
}

void SettingsScreen::BackButtonHandler(lv_event_t* e) {
  SettingsScreen* obj = (SettingsScreen*)lv_event_get_user_data(e);
  ConfirmationContext ctx{.parent_screen = obj->GetScreen(),
                          .backdrop = obj->_labels->backdrop,
                          .title = "Apply changes?",
                          .message = "Apply the new target temperature?",
                          .confirm_text = "Apply",
                          .cancel_text = "Cancel",
                          .on_confirm =
                              [obj]() {
                                FLOG_INFO("Applying new target temperature: %d°C", (int)obj->_pending_value);
                                // Publish new target temperature
                                // PS_PUB_INT("heater.target.temperature.set", obj->_pending_value);
                                PS_PUB_NIL("ui.action.running");

                                obj->_has_pending = false;
                                obj->_pending_value = 0;
                                if (obj->_labels->backdrop) lv_obj_delete(obj->_labels->backdrop);
                              },
                          .on_cancel =
                              [obj]() {
                                PS_PUB_NIL("ui.action.running");
                                FLOG_INFO("Cancelled applying new target temperature");
                                obj->_has_pending = false;
                                obj->_pending_value = 0;
                                if (obj->_labels->backdrop) lv_obj_delete(obj->_labels->backdrop);
                              }};

  ConfirmationPopup(ctx);
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

void SettingsScreen::NumpadOpenHandler(lv_event_t* e) {
  SettingsScreen* obj = (SettingsScreen*)lv_event_get_user_data(e);

  NumpadContext ctx{.parent_screen = obj->GetScreen(),
                    .backdrop = obj->_labels->backdrop,          // backdrop
                    .target_spinbox = obj->_labels->set_target,  // spinbox
                    .on_confirm = [obj](std::optional<int32_t> val) {
                      if (val.has_value() && !std::isnan(val.value())) {
                        FLOG_INFO("Value: %li", val.value());
                        PS_PUB_INT("heater.target.temperature.set", val.value());
                      } else {
                        PS_PUB_NIL("heater.target.temperature.set");
                      }
                      // lv_label_set_text(obj->_labels->set_target, )
                    }};
  NumpadOpen(ctx);
}

}  // namespace toothless
