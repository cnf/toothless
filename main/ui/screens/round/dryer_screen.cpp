#include "ui/screens/dryer_screen.hpp"

#include <cmath>

#include "funlog.h"
#include "heater/heater.hpp"
#include "ui/display/display.hpp"
#include "ui/screens/screen_helpers.hpp"
#include "ui/themes/style_registry.hpp"
#include "ui/themes/widget_factories.hpp"

extern "C" {
#include <pubsub.h>
}

// LV_FONT_DECLARE(AdwaitaMonoB_96);
// LV_FONT_DECLARE(AdwaitaMonoB_48);
// LV_FONT_DECLARE(AdwaitaMonoB_32);

namespace toothless {

DryerScreen::DryerScreen() { _labels = std::make_unique<DryerScreenLabels>(); }

DryerScreen::~DryerScreen() {
  if (_update_timer) {
    lv_timer_delete(_update_timer);
  }
  if (_subscription) {
    ps_free_subscriber(_subscription);
  }
}

lv_obj_t* DryerScreen::Create() {
  // esp_log_level_set(FLOG_SHORT_FILENAME, ESP_LOG_DEBUG);
  _subscription = ps_new_subscriber(10, PS_STRLIST("sensor.temperature.zone", "heater"));
  // FIXME: should probably make queue size configurable

  _screen = ui::CreateScreen();
  lv_obj_set_layout(_screen, LV_LAYOUT_FLEX);          // Set screen to vertical flex layout
  lv_obj_set_flex_flow(_screen, LV_FLEX_FLOW_COLUMN);  // Vertical stacking

  MainSection();
  _labels->start_stop_button = CreateBottomRow(_screen);

  _update_timer = lv_timer_create(UIUpdateTimerCB, kUIUpdateIntervalMs, this);  // Update every 100ms
  return _screen;
}
void DryerScreen::Loop() {}

void DryerScreen::UIUpdateTimerCB(lv_timer_t* timer) {
  DryerScreen* screen = (DryerScreen*)lv_timer_get_user_data(timer);
  if (screen) {
    screen->UpdateAllDisplays();
  }
}

esp_err_t DryerScreen::UpdateAllDisplays() {
  ps_msg_t* msg = nullptr;
  for ((msg = ps_get(_subscription, 0)); msg != NULL; (msg = ps_get(_subscription, 0))) {
    if (ps_has_topic(msg, "sensor.temperature.zone") && PS_IS_INT(msg)) {
      // FLOG_DEBUG("Received temperature: %d", (int)msg->int_val);
      TemperatureUpdateCurrent((uint32_t)msg->int_val);
    } else if (ps_has_topic(msg, "heater.state") && PS_IS_INT(msg)) {
      switch (msg->int_val) {
        case heater::kStateOn:
          lv_label_set_text(_labels->start_stop_button, "Stop");
          break;
        default:
          lv_label_set_text(_labels->start_stop_button, "Start");
          break;
      }
    } else if (ps_has_topic(msg, "heater.power") && PS_IS_BOOL(msg)) {
      // FLOG_INFO("Power: %d", msg->bool_val);
      switch (msg->bool_val) {
        case true:
          if (_labels->heater_led) ui::SetLEDState(_labels->heater_led, true);
          if (_labels->temperature_current)
            lv_obj_set_style_text_color(_labels->temperature_current, lv_palette_main(LV_PALETTE_RED), 0);
          break;
        case false:
          if (_labels->heater_led) ui::SetLEDState(_labels->heater_led, false);
          if (_labels->temperature_current)
            lv_obj_set_style_text_color(_labels->temperature_current, lv_color_white(), 0);

          break;
      }
    } else if (ps_has_topic(msg, "heater.target.temperature")) {
      if (PS_IS_INT(msg)) {
        TemperatureUpdateTarget((uint32_t)msg->int_val);
      } else {
        TemperatureClearTarget();
      }
    } else if (ps_has_topic(msg, "heater.timer.remaining")) {
      if (PS_IS_INT(msg)) {
        TimerUpdate((uint32_t)msg->int_val);
      } else {
        TimerClear();
      }
    } else if (ps_has_topic(msg, "sensor.somethingelse") && PS_IS_INT(msg)) {
      FLOG_ERROR("TODO");
    }
    ps_unref_msg(msg);
  }
  return ESP_OK;
}

esp_err_t DryerScreen::Temperature() {
  lv_obj_t* temp_container = ui::CreateRowContainer(_screen);
  // lv_obj_t* temp_container = lv_obj_create(_screen);
  lv_obj_add_flag(temp_container, LV_OBJ_FLAG_HIDDEN);  // Hide until we add more temps
  // lv_obj_set_style_pad_all(temp_container, 0, 0);       // Remove all padding
  // lv_obj_set_scrollbar_mode(temp_container, LV_SCROLLBAR_MODE_OFF);
  // lv_obj_remove_flag(temp_container, LV_OBJ_FLAG_SCROLLABLE);
  // lv_obj_set_size(temp_container, lv_pct(100), lv_pct(10));
  // lv_obj_set_layout(temp_container, LV_LAYOUT_FLEX);
  // lv_obj_set_flex_flow(temp_container, LV_FLEX_FLOW_ROW);  // Side by side
  // lv_obj_set_style_pad_gap(temp_container, 10, 0);         // Gap between temp blocks

  // _labels->temp_current = TemperatureBlock(temp_container, "Current", "--°C");
  lv_obj_t* temperature_obj = lv_obj_create(temp_container);
  lv_obj_set_style_pad_all(temperature_obj, 0, 0);  // Remove all padding

  lv_obj_set_flex_grow(temperature_obj, 1);  // Equal width temperature_objs
  lv_obj_set_layout(temperature_obj, LV_LAYOUT_FLEX);
  lv_obj_set_flex_flow(temperature_obj, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_style_pad_gap(temperature_obj, 2, 0);  // ← ADD THIS LINE - 2px gap instead of default
  lv_obj_set_flex_align(temperature_obj, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

  // // Title
  // lv_obj_t *cur_title_label = lv_label_create(temperature_obj);
  // lv_label_set_text(cur_title_label, title);
  // lv_obj_set_style_text_font(cur_title_label, &lv_font_montserrat_10, 0);
  // // lv_obj_set_size(cur_title_label, lv_pct(10), LV_SIZE_CONTENT); // Auto height

  // Temperature
  _labels->temperature_current = lv_label_create(temperature_obj);
  lv_label_set_text(_labels->temperature_current, "...");
  lv_obj_set_style_text_font(_labels->temperature_current, &lv_font_montserrat_22, 0);

  return ESP_OK;
}

void DryerScreen::TemperatureUpdateTarget(int32_t temp) {
  FLOG_INFO("yes?");
  if (!_labels->temperature_target) return;
  FLOG_VERBOSE("Received target: %d", temp);
  // char temp_str[16];
  int32_t clamped_temp = std::clamp<int32_t>(temp / 100, int32_t(-99), int32_t(999));
  lv_label_set_text_fmt(_labels->temperature_target, "%li", clamped_temp);
  // snprintf(temp_str, sizeof(temp_str), "%li°C", clamped_temp);
  // lv_label_set_text(_labels->temp_target, temp_str);
}

void DryerScreen::TemperatureClearTarget() { lv_label_set_text(_labels->temperature_target, ".."); }

void DryerScreen::TemperatureUpdateCurrent(int32_t temp) {
  if (!_labels->temperature_current) return;
  FLOG_VERBOSE("Received temperature: %d", temp);
  float ctemp = temp / 100.0f;
  char temp_str[16];

  float clamped_temp = std::clamp(ctemp, -999.99f, 9999.99f);
  snprintf(temp_str, sizeof(temp_str), "%.0f", clamped_temp);
  lv_label_set_text(_labels->temperature_current, temp_str);
  // lv_label_set_text_fmt(_labels->temperature_current, "%.1f°C", clamped_temp);
}
void DryerScreen::TimerUpdate(uint32_t seconds) {
  if (!_labels->timer) return;
  char hours[2] = {'\0'};
  // FLOG_INFO("Received timer: %d seconds", seconds);
  if (seconds > 3599) {
    seconds /= 60;  // show HH:MM when over an hour
    hours[0] = 'h';
    hours[1] = '\0';
  }
  uint16_t aa = seconds / 60;
  uint16_t bb = seconds % 60;
  lv_label_set_text_fmt(_labels->timer, "%s%02d:%02d", hours, aa, bb);

  // char temp_str[16];
  // snprintf(temp_str, sizeof(temp_str), "%02d:%02d", aa, bb);
  // FLOG_INFO("Updating timer display to %02d:%02d [%s]", aa, bb, temp_str);
  // lv_label_set_text(_labels->timer, temp_str);
}

void DryerScreen::TimerClear() { lv_label_set_text(_labels->timer, "00:00"); };

// void DryerScreen::UpdateTemperatureDisplay(uint32_t temp) {
//   FLOG_DEBUG("Received temperature: %d", temp);
//   // float temperature = temp / 100.0f;
//   static uint32_t last_temp = UINT32_MAX;
//   static char temp_str[16];

//   if (temp != last_temp) {
//     float temperature = temp / 100.0f;
//     snprintf(temp_str, sizeof(temp_str), "%.2f°C", temperature);
//     lv_label_set_text(_labels->temperature_current, temp_str);
//     last_temp = temp;
//   }
//   // lv_label_set_text(_labels->temperature, temp_str);
// }

void DryerScreen::MainSection() {
  lv_obj_t* wrapper = ui::CreateColumnContainer(_screen);
  // lv_obj_t* wrapper = lv_obj_create(_screen);
  // lv_obj_remove_style_all(wrapper);
  // lv_obj_set_style_bg_opa(wrapper, LV_OPA_TRANSP, 0);
  lv_obj_set_size(wrapper, lv_pct(100), LV_SIZE_CONTENT);
  lv_obj_set_flex_grow(wrapper, 1);  // This wrapper grows to fill remaining space
  // lv_obj_set_layout(wrapper, LV_LAYOUT_FLEX);
  // lv_obj_set_flex_flow(wrapper, LV_FLEX_FLOW_COLUMN);
  // lv_obj_set_style_pad_all(wrapper, 0, 0);
  // lv_obj_set_style_pad_gap(wrapper, 10, 0);  // Gap between temp blocks

  CreateTemperature(wrapper);
  CreateTimer(wrapper);
};

void DryerScreen::CreateTemperature(lv_obj_t* parent) {
  // Temperature row

  lv_obj_t* wrapper = ui::CreateRowContainer(parent);
  lv_obj_set_size(wrapper, lv_pct(100), LV_SIZE_CONTENT);

  lv_obj_set_flex_align(wrapper, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START);
  lv_obj_set_flex_grow(wrapper, 0);
  // lv_obj_set_style_pad_all(wrapper, 25, 0);

  {
    lv_obj_t* taco = ui::CreateRowContainer(wrapper);
    lv_obj_set_size(taco, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_flex_align(taco, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);

    _labels->temperature_current = ui::CreateValueLarge(taco, 00.0f, "%.0f");
    lv_obj_set_style_text_font(_labels->temperature_current, &themes::fonts::numbers_large, 0);
    lv_obj_t* unit = ui::CreateUnitLabel(taco, "°C");
    lv_obj_set_style_text_font(unit, &themes::fonts::numbers_small, 0);
  }

  // HeaterLED(wrapper);
}

void DryerScreen::CreateTimer(lv_obj_t* parent) {
  lv_obj_t* wrapper = ui::CreateRowContainer(parent);

  lv_obj_set_size(wrapper, lv_pct(100), LV_SIZE_CONTENT);
  lv_obj_set_flex_align(wrapper, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

  lv_obj_set_flex_grow(wrapper, 1);
  {
    lv_obj_t* taco = ui::CreateRowContainer(wrapper);
    // lv_obj_t* taco = lv_obj_create(wrapper);
    lv_obj_remove_style_all(taco);
    lv_obj_set_style_bg_opa(taco, LV_OPA_TRANSP, 0);
    lv_obj_set_style_pad_all(taco, 0, 0);

    lv_obj_set_size(taco, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_layout(taco, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(taco, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(taco, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);

    _labels->temperature_target = lv_label_create(taco);
    lv_label_set_text(_labels->temperature_target, "...");
    lv_obj_set_style_text_font(_labels->temperature_target, &themes::fonts::numbers_medium, 0);

    lv_obj_t* target_unit = lv_label_create(taco);
    lv_label_set_text(target_unit, "°C");
    lv_obj_set_style_text_font(target_unit, &themes::fonts::numbers_small, 0);
    lv_obj_set_align(target_unit, LV_ALIGN_TOP_LEFT);

    lv_obj_add_flag(taco, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(taco, TargetHandler, LV_EVENT_CLICKED, this);
  }

  _labels->timer = lv_label_create(wrapper);
  lv_label_set_text(_labels->timer, "00:00");
  lv_obj_set_style_text_font(_labels->timer, &themes::fonts::numbers_medium, 0);
  lv_obj_align(_labels->timer, LV_ALIGN_RIGHT_MID, 0, 0);
  lv_obj_add_flag(_labels->timer, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_add_event_cb(_labels->timer, TimerHandler, LV_EVENT_CLICKED, this);
}

void DryerScreen::TimerHandler(lv_event_t* e) {
  DryerScreen* obj = (DryerScreen*)lv_event_get_user_data(e);

  NumberRollerContext ctx{.parent_screen = obj->GetScreen(),
                          .backdrop = obj->_labels->backdrop,            // backdrop
                          .target_spinbox = obj->_labels->timer_target,  // spinbox
                          .on_confirm = [obj](std::optional<int32_t> val) {
                            if (val.has_value() && !std::isnan(val.value())) {
                              FLOG_INFO("Value: %li", val.value());
                              PS_PUB_INT("heater.timer.set", val.value());
                            } else {
                              PS_PUB_NIL("heater.timer.set");
                            }
                            // lv_label_set_text(obj->_labels->set_target, )
                          }};

  TimeRollerOpen(ctx);
}

void DryerScreen::TargetHandler(lv_event_t* e) {
  DryerScreen* obj = (DryerScreen*)lv_event_get_user_data(e);

  NumpadContext ctx{.parent_screen = obj->GetScreen(),
                    .backdrop = obj->_labels->backdrop,          // backdrop
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

void DryerScreen::HeaterLED(lv_obj_t* parent) {
  _labels->heater_led = ui::CreateLEDIndicator(parent, false);
  lv_obj_set_size(_labels->heater_led, 65, 65);
  // lv_led_set_color(_labels->heater_led, lv_palette_main(LV_PALETTE_RED));
}

}  // namespace toothless
