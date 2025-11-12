#include "ui/screens/dryer_screen.hpp"

#include <cmath>

#include "funlog.h"
#include "heater/heater.hpp"
#include "ui/display/display.hpp"
#include "ui/screens/screen_helpers.hpp"

extern "C" {
#include <pubsub.h>
}

LV_FONT_DECLARE(AdwaitaMonoB_96);
LV_FONT_DECLARE(AdwaitaMonoB_48);
LV_FONT_DECLARE(AdwaitaMonoB_32);

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
  _subscription = ps_new_subscriber(10, PS_STRLIST("sensor.temperature.chamber", "heater"));
  // FIXME: should probably make queue size configurable
  _screen = lv_obj_create(NULL);
  lv_obj_set_style_bg_color(_screen, lv_color_black(), 0);
  lv_obj_set_style_pad_all(_screen, 10, 0);            // Global 2% border
  lv_obj_set_layout(_screen, LV_LAYOUT_FLEX);          // Set screen to vertical flex layout
  lv_obj_set_flex_flow(_screen, LV_FLEX_FLOW_COLUMN);  // Vertical stacking
  // lv_obj_set_style_pad_gap(_screen, 10, 0);            // 10px gap between items

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
    if (ps_has_topic(msg, "sensor.temperature.chamber") && PS_IS_INT(msg)) {
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
      if (!_labels->heater_led) continue;
      switch (msg->bool_val) {
        case true:
          lv_led_on(_labels->heater_led);
          break;
        case false:
          lv_led_off(_labels->heater_led);
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
  lv_obj_t* temp_container = lv_obj_create(_screen);
  lv_obj_add_flag(temp_container, LV_OBJ_FLAG_HIDDEN);  // Hide until we add more temps
  lv_obj_set_style_pad_all(temp_container, 0, 0);       // Remove all padding
  lv_obj_set_scrollbar_mode(temp_container, LV_SCROLLBAR_MODE_OFF);
  lv_obj_remove_flag(temp_container, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_size(temp_container, lv_pct(100), lv_pct(10));
  lv_obj_set_layout(temp_container, LV_LAYOUT_FLEX);
  lv_obj_set_flex_flow(temp_container, LV_FLEX_FLOW_ROW);  // Side by side
  lv_obj_set_style_pad_gap(temp_container, 10, 0);         // Gap between temp blocks

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
  lv_label_set_text(_labels->temperature_current, "----");
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

void DryerScreen::TemperatureClearTarget() { lv_label_set_text(_labels->temperature_target, "--"); }

void DryerScreen::TemperatureUpdateCurrent(int32_t temp) {
  if (!_labels->temperature_current) return;
  FLOG_VERBOSE("Received temperature: %d", temp);
  float ctemp = temp / 100.0f;
  char temp_str[16];

  float clamped_temp = std::clamp(ctemp, -999.99f, 9999.99f);
  snprintf(temp_str, sizeof(temp_str), "%.1f", clamped_temp);
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

void DryerScreen::TimerClear() { lv_label_set_text(_labels->timer, "--:--"); };

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
  lv_obj_t* wrapper = lv_obj_create(_screen);
  lv_obj_remove_style_all(wrapper);
  lv_obj_set_style_bg_opa(wrapper, LV_OPA_TRANSP, 0);
  lv_obj_set_size(wrapper, lv_pct(100), 0);
  lv_obj_set_flex_grow(wrapper, 1);  // This wrapper grows to fill remaining space
  lv_obj_set_layout(wrapper, LV_LAYOUT_FLEX);
  lv_obj_set_flex_flow(wrapper, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_style_pad_all(wrapper, 0, 0);
  // lv_obj_set_style_pad_gap(wrapper, 10, 0);  // Gap between temp blocks

  CreateTemperature(wrapper);
  CreateTimer(wrapper);
};

void DryerScreen::CreateTemperature(lv_obj_t* parent) {
  // Temperature row

  lv_obj_t* wrapper = lv_obj_create(parent);
  lv_obj_remove_style_all(wrapper);
  lv_obj_set_style_bg_opa(wrapper, LV_OPA_TRANSP, 0);
  lv_obj_set_size(wrapper, lv_pct(100), LV_SIZE_CONTENT);
  lv_obj_set_layout(wrapper, LV_LAYOUT_FLEX);
  lv_obj_set_flex_flow(wrapper, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(wrapper, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START);
  lv_obj_set_flex_grow(wrapper, 0);
  lv_obj_set_style_pad_all(wrapper, 25, 0);

  // lv_obj_set_style_border_width(wrapper, 1, 0);
  {
    lv_obj_t* taco = lv_obj_create(wrapper);
    lv_obj_remove_style_all(taco);
    lv_obj_set_style_bg_opa(taco, LV_OPA_TRANSP, 0);
    lv_obj_set_style_pad_all(taco, 0, 0);

    lv_obj_set_size(taco, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_layout(taco, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(taco, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(taco, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);

    lv_obj_t* temperature = lv_label_create(taco);
    lv_label_set_text(temperature, "24.89");
    lv_obj_set_style_text_font(temperature, &AdwaitaMonoB_96, 0);
    _labels->temperature_current = temperature;
    lv_obj_t* unit = lv_label_create(taco);
    lv_label_set_text(unit, "°C");
    lv_obj_set_style_text_font(unit, &AdwaitaMonoB_32, 0);
  }

  HeaterLED(wrapper);
}

void DryerScreen::CreateTimer(lv_obj_t* parent) {
  lv_obj_t* wrapper = lv_obj_create(parent);
  lv_obj_remove_style_all(wrapper);
  lv_obj_set_style_bg_opa(wrapper, LV_OPA_TRANSP, 0);
  // lv_obj_set_style_pad_all(wrapper, 25, 0);
  // lv_obj_set_style_pad_gap(wrapper, 10, 0);
  lv_obj_set_size(wrapper, lv_pct(100), LV_SIZE_CONTENT);

  lv_obj_set_layout(wrapper, LV_LAYOUT_FLEX);
  lv_obj_set_flex_flow(wrapper, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(wrapper, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

  lv_obj_set_flex_grow(wrapper, 1);
  {
    lv_obj_t* taco = lv_obj_create(wrapper);
    lv_obj_remove_style_all(taco);
    lv_obj_set_style_bg_opa(taco, LV_OPA_TRANSP, 0);
    lv_obj_set_style_pad_all(taco, 0, 0);

    lv_obj_set_size(taco, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_layout(taco, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(taco, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(taco, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);

    _labels->temperature_target = lv_label_create(taco);
    lv_label_set_text(_labels->temperature_target, "---");
    lv_obj_set_style_text_font(_labels->temperature_target, &AdwaitaMonoB_48, 0);

    lv_obj_t* target_unit = lv_label_create(taco);
    lv_label_set_text(target_unit, "°C");
    lv_obj_set_style_text_font(target_unit, &AdwaitaMonoB_32, 0);
    lv_obj_set_align(target_unit, LV_ALIGN_TOP_LEFT);

    lv_obj_add_flag(taco, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(taco, TargetHandler, LV_EVENT_CLICKED, this);
  }

  _labels->timer = lv_label_create(wrapper);
  lv_label_set_text(_labels->timer, "00:00");
  lv_obj_set_style_text_font(_labels->timer, &AdwaitaMonoB_48, 0);
  lv_obj_align(_labels->timer, LV_ALIGN_RIGHT_MID, 0, 0);
  lv_obj_add_flag(_labels->timer, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_add_event_cb(_labels->timer, TimerHandler, LV_EVENT_CLICKED, this);
}

void DryerScreen::TimerHandler(lv_event_t* e) {
  DryerScreen* obj = (DryerScreen*)lv_event_get_user_data(e);

  TimeRollerContext ctx{.parent_screen = obj->GetScreen(),
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
  // FLOG_INFO("TargetHandler called");
  // TimeRollerContext ctx = {
  //     .parent_screen = obj->GetScreen(),
  //     .backdrop = obj->_labels->backdrop,            // backdrop
  //     .target_spinbox = obj->_labels->temperature_target,  // spinbox
  //     .on_confirm = [obj](std::optional<int32_t> val) {
  //       if (val.has_value() && !std::isnan(val.value())) {
  //         FLOG_INFO("Value: %li", val.value());
  //         PS_PUB_INT("heater.target.temperature", val.value());
  //       } else {
  //         PS_PUB_NIL("heater.target.temperature");
  //       }
  //       // lv_label_set_text(obj->_labels->set_target, )
  //     }};

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

void DryerScreen::HeaterLED(lv_obj_t* parent) {
  _labels->heater_led = lv_led_create(parent);
  lv_obj_align(_labels->heater_led, LV_ALIGN_CENTER, 0, 0);
  lv_obj_set_size(_labels->heater_led, 65, 65);
  lv_obj_set_style_radius(_labels->heater_led, LV_RADIUS_CIRCLE, 0);

  lv_led_off(_labels->heater_led);
  lv_led_set_color(_labels->heater_led, lv_palette_main(LV_PALETTE_RED));
}

esp_err_t DryerScreen::BottomRow() {
  lv_obj_t* temp_container;
  {
    temp_container = lv_obj_create(_screen);
    lv_obj_remove_style_all(temp_container);  // Make it invisible
    lv_obj_set_style_bg_opa(temp_container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_pad_all(temp_container, 0, 0);  // Remove all padding
    lv_obj_set_scrollbar_mode(temp_container, LV_SCROLLBAR_MODE_OFF);
    lv_obj_remove_flag(temp_container, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_size(temp_container, lv_pct(100), lv_pct(15));
    lv_obj_set_layout(temp_container, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(temp_container, LV_FLEX_FLOW_ROW);  // Side by side
    lv_obj_set_style_pad_gap(temp_container, 10, 0);         // Gap between temp blocks
    // lv_obj_set_flex_align(temp_container, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_END);
  }
  StartButton(temp_container);
  return ESP_OK;
}

/// @brief Button to start reflow
/// @return
esp_err_t DryerScreen::StartButton(lv_obj_t* container) {
  lv_obj_t* start_btn = lv_button_create(container);
  lv_obj_set_size(start_btn, lv_pct(50), lv_pct(100));
  // lv_obj_align(start_btn, LV_ALIGN_BOTTOM_LEFT, lv_pct(2), lv_pct(-2));

  lv_obj_t* start_label = lv_label_create(start_btn);
  lv_label_set_text(start_label, "Start");
  lv_obj_center(start_label);
  lv_obj_set_flex_grow(start_btn, 1);  // Equal width temperature_objs

  // Register event with screen object as user data
  lv_obj_add_event_cb(start_btn, StartButtonEventHandler, LV_EVENT_CLICKED, this);
  return ESP_OK;
}

void DryerScreen::StartButtonEventHandler(lv_event_t* e) {
  lv_event_code_t code = lv_event_get_code(e);

  // lv_obj_t *btn = (lv_obj_t *)lv_event_get_target(e);

  if (code == LV_EVENT_CLICKED) {
    FLOG_DEBUG("Start Button clicked");

    // Get the screen object if you passed it as user_data
    DryerScreen* screen = (DryerScreen*)lv_event_get_user_data(e);
    if (screen) {
      screen->HandleStartButtonPress();
    }
  }
}

void DryerScreen::HandleStartButtonPress() {
  // TODO: this can probably just live in StartButtonEventHandler
  PS_PUB_NIL("ui.action.start");
  // Switch to running screen
  // userInterface->SwitchTo(ScreenList::kReflowScreen);
}

}  // namespace toothless
