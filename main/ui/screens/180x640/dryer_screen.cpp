#include "ui/screens/dryer_screen.hpp"

#include <cmath>

#include "funlog.h"
#include "heater/heater.hpp"
#include "local_helpers.hpp"
#include "ui/display/display.hpp"
#include "ui/screens/screen_helpers.hpp"
#include "ui/themes/widget_factories.hpp"

extern "C" {
#include <pubsub.h>
}

LV_FONT_DECLARE(AdwaitaMonoB_128);
LV_FONT_DECLARE(AdwaitaMonoB_96);
LV_FONT_DECLARE(AdwaitaMonoB_48);
LV_FONT_DECLARE(AdwaitaMonoB_32);
LV_FONT_DECLARE(AdwaitaMonoB_28);

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

  _screen = ui::CreateScreen();

  _subjects = SubjectManager::Instance().subjects;

  _labels->left = ui::CreateSubScreen(_screen);
  lv_obj_set_size(_labels->left, lv_pct(50), lv_pct(100));

  _labels->right = ui::CreateSubScreen(_screen);
  lv_obj_set_size(_labels->right, lv_pct(50), lv_pct(100));

  lv_obj_set_layout(_labels->right, LV_LAYOUT_FLEX);
  lv_obj_set_flex_flow(_labels->right, LV_FLEX_FLOW_COLUMN);

  // lv_obj_set_style_border_width(_labels->right, 2, 0);
  // lv_obj_set_style_border_color(_labels->right, lv_color_hex(0x999900), 0);
  // lv_obj_set_style_pad_all(_labels->right, 0, 0);

  lv_obj_set_x(_labels->right, lv_pct(50));  // 320);

  CreateTemperature(_labels->left);

  CreateTimer(_labels->right);
  _labels->start_stop_button = CreateBottomRow(_labels->right);
  lv_label_bind_text(_labels->start_stop_button, &_subjects->start_stop, "%s");

  _update_timer = lv_timer_create(UIUpdateTimerCB, kUIUpdateIntervalMs, this);

  // lv_subject_add_observer(
  //     &_subjects->heater_state,
  //     [](lv_observer_t* observer, lv_subject_t* subject) {
  //       DryerScreen* screen = (DryerScreen*)lv_observer_get_user_data(observer);
  //       if (screen) {
  //         int32_t state = lv_subject_get_int(subject);
  //         switch (state) {
  //           case heater::kStateOn:
  //             lv_label_set_text(screen->_labels->start_stop_button, "Stop");
  //             break;
  //           default:
  //             lv_label_set_text(screen->_labels->start_stop_button, "Start");
  //             break;
  //         }
  //       }
  //     },
  //     nullptr);
  // lv_label_set_text(_labels->start_stop_button, "Stop");
  // lv_label_set_text(_labels->start_stop_button, "Start");
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
  return ESP_OK;
  FLOG_ERROR("Use subjects instead");
  ps_msg_t* msg = nullptr;
  for ((msg = ps_get(_subscription, 0)); msg != NULL; (msg = ps_get(_subscription, 0))) {
    if (ps_has_topic(msg, "sensor.temperature.chamber") && PS_IS_INT(msg)) {
      // FLOG_DEBUG("Received temperature: %d", (int)msg->int_val);
      // TemperatureUpdateCurrent((uint32_t)msg->int_val);
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
          // if (_labels->heater_led) ui::SetLEDState(_labels->heater_led, true);
          // if (_labels->temperature_current)
          // lv_obj_set_style_text_color(_labels->temperature_current, lv_palette_main(LV_PALETTE_RED), 0);
          break;
        case false:
          // if (_labels->heater_led) ui::SetLEDState(_labels->heater_led, false);
          // if (_labels->temperature_current)
          // lv_obj_set_style_text_color(_labels->temperature_current, lv_color_white(), 0);
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

void DryerScreen::TemperatureUpdateTarget(int32_t temp) {
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
  return;
  // if (!_labels->temperature_current) return;
  // FLOG_VERBOSE("Received temperature: %d", temp);
  // float ctemp = temp / 100.0f;
  // char temp_str[16];
  // float clamped_temp = std::clamp(ctemp, -999.99f, 9999.99f);
  // snprintf(temp_str, sizeof(temp_str), "%.f", clamped_temp);
  // lv_label_set_text(_labels->temperature_current, temp_str);
}

// static void TimerUpdateCB(lv_observer_t* observer, lv_subject_t* subject) {
//   int32_t v = lv_subject_get_int(subject);
//   DryerScreen* screen = (DryerScreen*)lv_observer_get_user_data(observer);
//   if (screen) {
//     int32_t seconds = lv_subject_get_int(subject);
//     screen->TimerUpdate((uint32_t)seconds);
//   }
// }
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
}

void DryerScreen::TimerClear() { lv_label_set_text(_labels->timer, "00:00"); };

void DryerScreen::MainSection() {
  // lv_obj_t* wrapper = ui::CreateRowContainer(_screen);
  // lv_obj_set_style_flex_main_place(wrapper, LV_FLEX_ALIGN_SPACE_BETWEEN, 0);

  // CreateTemperature(wrapper);
  // CreateTimer(wrapper);
};

void DryerScreen::CreateTemperature(lv_obj_t* parent) {
  lv_obj_t* wrapper = ui::CreateRowContainer(parent);
  lv_obj_set_size(wrapper, lv_pct(100), LV_SIZE_CONTENT);
  lv_obj_set_style_flex_track_place(wrapper, LV_FLEX_ALIGN_CENTER, 0);
  lv_obj_set_style_pad_gap(wrapper, 1, 0);

  // lv_obj_set_flex_align(wrapper, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
  lv_obj_set_flex_align(wrapper, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER);

  _labels->temperature_current = ui::CreateValueLarge(wrapper, 00.0f, "%.f");
  lv_label_bind_text(_labels->temperature_current, &_subjects->temperature, "%li");
  lv_obj_bind_state_if_not_eq(_labels->temperature_current, &_subjects->heater_power, LV_STATE_USER_1, 0);

  lv_obj_t* unit = ui::CreateUnitLabel(wrapper, "°C");
  lv_obj_set_align(unit, LV_ALIGN_TOP_LEFT);
}

void DryerScreen::CreateTimer(lv_obj_t* parent) {
  lv_obj_t* wrapper = ui::CreateRowContainer(parent);
  // lv_obj_set_style_flex_track_place(wrapper, LV_FLEX_ALIGN_CENTER, 0);

  lv_obj_set_size(wrapper, lv_pct(100), LV_SIZE_CONTENT);
  lv_obj_set_flex_align(wrapper, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

  lv_obj_set_flex_grow(wrapper, 1);
  {
    lv_obj_t* taco = ui::CreateRowContainer(wrapper);
    lv_obj_set_style_pad_gap(taco, 1, 0);

    lv_obj_set_size(taco, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_flex_align(taco, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);

    _labels->temperature_target = ui::CreateValueSmall(taco, 00.0f, "%.0f");
    // lv_label_set_text(_labels->temperature_target, "--");
    lv_label_bind_text(_labels->temperature_target, &_subjects->target, "%li");

    lv_obj_t* target_unit = ui::CreateUnitLabel(taco, "°C");
    lv_obj_set_align(target_unit, LV_ALIGN_TOP_LEFT);

    lv_obj_add_flag(taco, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(taco, TargetHandler, LV_EVENT_CLICKED, this);
  }

  // HeaterLED(wrapper);

  _labels->timer = ui::CreateValueSmall(wrapper, 0, "%02d:%02d");
  // lv_label_set_text(_labels->timer, "00:00");
  lv_obj_align(_labels->timer, LV_ALIGN_RIGHT_MID, 0, 0);
  lv_obj_add_flag(_labels->timer, LV_OBJ_FLAG_CLICKABLE);
  lv_label_bind_text(_labels->timer, &_subjects->timer_string, "%s");
  lv_obj_add_event_cb(_labels->timer, TimerHandler, LV_EVENT_CLICKED, this);
}

void DryerScreen::TimerHandler(lv_event_t* e) {
  DryerScreen* obj = (DryerScreen*)lv_event_get_user_data(e);

  NumberRollerContext ctx{.parent_screen = obj->_labels->right,
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
  NumberRollerContext ctx{.parent_screen = obj->_labels->right,
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

  LocalTempRollerOpen(ctx);
}

void DryerScreen::HeaterLED(lv_obj_t* parent) { _labels->heater_led = ui::CreateLEDIndicator(parent, false); }

// esp_err_t DryerScreen::BottomRow() {
//   lv_obj_t* temp_container;
//   {
//     temp_container = lv_obj_create(_screen);
//     lv_obj_remove_style_all(temp_container);  // Make it invisible
//     lv_obj_set_style_bg_opa(temp_container, LV_OPA_TRANSP, 0);
//     lv_obj_set_style_pad_all(temp_container, 0, 0);  // Remove all padding
//     lv_obj_set_scrollbar_mode(temp_container, LV_SCROLLBAR_MODE_OFF);
//     lv_obj_remove_flag(temp_container, LV_OBJ_FLAG_SCROLLABLE);
//     lv_obj_set_size(temp_container, lv_pct(100), lv_pct(15));
//     lv_obj_set_layout(temp_container, LV_LAYOUT_FLEX);
//     lv_obj_set_flex_flow(temp_container, LV_FLEX_FLOW_ROW);  // Side by side
//     lv_obj_set_style_pad_gap(temp_container, 10, 0);         // Gap between temp blocks
//     // lv_obj_set_flex_align(temp_container, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_END);
//   }
//   StartButton(temp_container);
//   return ESP_OK;
// }

// /// @brief Button to start reflow
// /// @return
// esp_err_t DryerScreen::StartButton(lv_obj_t* container) {
//   lv_obj_t* start_btn = lv_button_create(container);
//   lv_obj_set_size(start_btn, lv_pct(50), lv_pct(100));
//   // lv_obj_align(start_btn, LV_ALIGN_BOTTOM_LEFT, lv_pct(2), lv_pct(-2));

//   lv_obj_t* start_label = lv_label_create(start_btn);
//   lv_label_set_text(start_label, "Start");
//   lv_obj_center(start_label);
//   lv_obj_set_flex_grow(start_btn, 1);  // Equal width temperature_objs

//   // Register event with screen object as user data
//   lv_obj_add_event_cb(start_btn, StartButtonEventHandler, LV_EVENT_CLICKED, this);
//   return ESP_OK;
// }

// void DryerScreen::StartButtonEventHandler(lv_event_t* e) {
//   lv_event_code_t code = lv_event_get_code(e);

//   // lv_obj_t *btn = (lv_obj_t *)lv_event_get_target(e);

//   if (code == LV_EVENT_CLICKED) {
//     FLOG_DEBUG("Start Button clicked");

//     // Get the screen object if you passed it as user_data
//     DryerScreen* screen = (DryerScreen*)lv_event_get_user_data(e);
//     if (screen) {
//       screen->HandleStartButtonPress();
//     }
//   }
// }

// void DryerScreen::HandleStartButtonPress() {
//   // TODO: this can probably just live in StartButtonEventHandler
//   PS_PUB_NIL("ui.action.start");
//   // Switch to running screen
//   // userInterface->SwitchTo(ScreenList::kReflowScreen);
// }

}  // namespace toothless
