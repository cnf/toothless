#include <cmath>

#include "dryer_screen.hpp"
#include "heater/heater.hpp"
#include "ui/screens/screen_helpers.hpp"
#include "ui/themes/widget_factories.hpp"

namespace toothless {

DryerScreen::DryerScreen() {
  _screen = ui::CreateScreen();
  _labels = std::make_unique<DryerScreenLabels>();
  _subjects = SubjectManager::Instance().subjects;
}

DryerScreen::~DryerScreen() {}

void DryerScreen::CreateTemperature(lv_obj_t* parent) {
  lv_obj_t* wrapper = ui::CreateRowContainer(parent);
  lv_obj_set_size(wrapper, lv_pct(100), LV_SIZE_CONTENT);
  lv_obj_set_style_flex_track_place(wrapper, LV_FLEX_ALIGN_CENTER, 0);
  lv_obj_set_style_pad_gap(wrapper, 1, 0);

  // lv_obj_set_flex_align(wrapper, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
  lv_obj_set_flex_align(wrapper, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER);

  lv_obj_t* temperature_current = ui::CreateValueLarge(wrapper, 00.0f, "%.f");
  lv_label_bind_text(temperature_current, &_subjects->temperature, "%li");
  lv_obj_bind_state_if_not_eq(temperature_current, &_subjects->heater_power, LV_STATE_USER_1, 0);

  lv_obj_t* unit = ui::CreateUnitLabel(wrapper, "°C");
  lv_obj_set_align(unit, LV_ALIGN_TOP_LEFT);
  // StatusBar(wrapper);
}

void DryerScreen::CreateTimerTargetRow(lv_obj_t* parent) {
  lv_obj_t* wrapper = ui::CreateRowContainer(parent);
  // lv_obj_set_style_flex_track_place(wrapper, LV_FLEX_ALIGN_CENTER, 0);

  lv_obj_set_size(wrapper, lv_pct(100), LV_SIZE_CONTENT);
  lv_obj_set_flex_align(wrapper, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

  lv_obj_set_flex_grow(wrapper, 1);

  CreateTarget(wrapper);
  CreateTimer(wrapper);
}

lv_obj_t* DryerScreen::CreateTarget(lv_obj_t* parent) {
  lv_obj_t* taco = ui::CreateRowContainer(parent);
  lv_obj_set_style_pad_gap(taco, 1, 0);

  lv_obj_set_size(taco, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
  lv_obj_set_flex_align(taco, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);

  lv_obj_t* temperature_target = ui::CreateValueSmall(taco, 00.0f, "%.0f");
  lv_subject_add_observer_obj(
      &_subjects->target,
      [](lv_observer_t* observer, lv_subject_t* subject) {
        lv_obj_t* label = static_cast<lv_obj_t*>(lv_observer_get_target(observer));
        int32_t temp = lv_subject_get_int(subject);
        if (temp <= -99 || temp >= 999) {
          lv_label_set_text(label, "--");
        } else {
          int32_t clamped_temp = std::clamp<int32_t>(temp, int32_t(-99), int32_t(999));
          lv_label_set_text_fmt(label, "%li", clamped_temp);
        }
      },
      temperature_target, NULL);

  lv_obj_t* target_unit = ui::CreateUnitLabel(taco, "°C");
  lv_obj_set_align(target_unit, LV_ALIGN_TOP_LEFT);

  lv_obj_add_flag(taco, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_add_event_cb(taco, TargetHandler, LV_EVENT_CLICKED, _screen);
  return temperature_target;
}

lv_obj_t* DryerScreen::CreateTimer(lv_obj_t* parent) {
  auto subjects = SubjectManager::Instance().subjects;
  lv_obj_t* timer = ui::CreateValueSmall(parent, 0, "%02d:%02d");
  lv_obj_align(timer, LV_ALIGN_RIGHT_MID, 0, 0);
  lv_obj_add_flag(timer, LV_OBJ_FLAG_CLICKABLE);
  lv_label_bind_text(timer, &subjects->timer_string, "%s");
  lv_obj_add_event_cb(timer, TimerHandler, LV_EVENT_CLICKED, _screen);
  return timer;
}

void DryerScreen::TimerHandler(lv_event_t* e) {
  lv_obj_t* target = (lv_obj_t*)lv_event_get_user_data(e);

  NumberRollerContext ctx{.parent_screen = target, .on_confirm = [](std::optional<int32_t> val) {
                            if (val.has_value() && !std::isnan(val.value())) {
                              FLOG_DEBUG("Value: %li", val.value());
                              PS_PUB_INT(topics::heater::timer_set, val.value());
                            } else {
                              PS_PUB_NIL(topics::heater::timer_set);
                            }
                          }};

  TimeRollerOpen(ctx);
}

void DryerScreen::TargetHandler(lv_event_t* e) {
  lv_obj_t* target = (lv_obj_t*)lv_event_get_user_data(e);

  NumpadContext ctx{.parent_screen = target, .on_confirm = [](std::optional<int32_t> val) {
                      if (val.has_value() && !std::isnan(val.value())) {
                        FLOG_DEBUG("Value: %li", val.value());
                        PS_PUB_INT(topics::heater::target_temperature_set, val.value() * 100);
                      } else {
                        PS_PUB_NIL(topics::heater::target_temperature_set);
                      }
                    }};

  NumpadOpen(ctx);
}

}  // namespace toothless
