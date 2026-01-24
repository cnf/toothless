#include "subjects.hpp"

#include <format>

#include "heater/heater.hpp"
#include "ui/themes/widget_factories.hpp"

extern "C" {
#include <pubsub.h>
}

namespace toothless {

SubjectManager::SubjectManager() {
  subjects = std::make_shared<Subjects>();
  lv_subject_init_int(&subjects->temperature, -100);
  lv_subject_init_int(&subjects->target, -100);
  lv_subject_init_int(&subjects->probe, -100);
  lv_subject_init_int(&subjects->heater_power, 0);
  lv_subject_init_int(&subjects->heater_state, 0);
  lv_subject_init_int(&subjects->sidebar, true);
  lv_subject_set_min_value_int(&subjects->sidebar, 0);
  lv_subject_set_max_value_int(&subjects->sidebar, 1);

  static char profile_buf[64];
  lv_subject_init_string(&subjects->profile, profile_buf, NULL, 64, "No Profile Loaded");
  lv_subject_init_int(&subjects->show_profile, 0);

  static char stage_buf[64];
  lv_subject_init_string(&subjects->stage, stage_buf, NULL, 64, "No Stage Loaded");
  lv_subject_init_int(&subjects->show_stage, 0);

  static char timer_buf[16];
  lv_subject_init_string(&subjects->timer_string, timer_buf, NULL, 16, "00:00");
  lv_subject_init_int(&subjects->timer_remaining, -1);

  static char start_stop_buf[16];
  lv_subject_init_string(&subjects->start_stop, start_stop_buf, NULL, 16, "Start");
}

SubjectManager::~SubjectManager() {
  if (_subscription) ps_free_subscriber(_subscription);
}

void SubjectManager::Init() {
  _subscription = ps_new_subscriber(10, PS_STRLIST("sensor.temperature", topics::heater::name));
}

void SubjectManager::Loop() {
  ps_msg_t* msg = nullptr;
  for ((msg = ps_get(_subscription, 0)); msg != NULL; (msg = ps_get(_subscription, 0))) {
    if (ps_has_topic(msg, "sensor.temperature.zone") && PS_IS_INT(msg)) {
      lv_subject_set_int(&subjects->temperature, (int32_t)msg->int_val / 100);
    } else if (ps_has_topic(msg, "sensor.temperature.probe") && PS_IS_INT(msg)) {
      // FLOG_INFO("Probe Temperature: %d", (int)msg->int_val);
      lv_subject_set_int(&subjects->probe, (int32_t)msg->int_val / 100);
    } else if (ps_has_topic(msg, topics::heater::target_temperature)) {
      if (PS_IS_INT(msg)) {
        lv_subject_set_int(&subjects->target, (int32_t)msg->int_val / 100);
      } else {
        lv_subject_set_int(&subjects->target, -100);
      }
    } else if (ps_has_topic(msg, topics::heater::power) && PS_IS_BOOL(msg)) {
      // FLOG_INFO("Power: %d", msg->bool_val);
      switch (msg->bool_val) {
        break;
        case true:
          lv_subject_set_int(&subjects->heater_power, 1);
          break;
        case false:
          lv_subject_set_int(&subjects->heater_power, 0);
          break;
      }
    } else if (ps_has_topic(msg, topics::heater::state) && PS_IS_INT(msg)) {
      lv_subject_set_int(&subjects->heater_state, msg->int_val);
      if (msg->int_val == heater::kStateOff) {
        lv_subject_copy_string(&subjects->start_stop, "Start");
      } else {
        lv_subject_copy_string(&subjects->start_stop, "Stop");
      }
    } else if (ps_has_topic(msg, topics::heater::profile_stage)) {
      if (PS_IS_STR(msg)) {
        if (lv_subject_get_int(&subjects->show_profile) == 0) PS_PUB_NIL(topics::heater::profile_get);
        lv_subject_copy_string(&subjects->stage, ui::SnakeToTitle(msg->str_val).c_str());
        lv_subject_set_int(&subjects->show_stage, 1);
      } else if (PS_IS_NIL(msg)) {
        lv_subject_set_int(&subjects->show_stage, 0);
        lv_subject_copy_string(&subjects->stage, "No Stage Loaded");
      }
    } else if (ps_has_topic(msg, topics::heater::profile)) {
      if (PS_IS_STR(msg)) {
        lv_subject_copy_string(&subjects->profile, ui::SnakeToTitle(msg->str_val).c_str());
        lv_subject_set_int(&subjects->show_profile, 1);
      } else if (PS_IS_NIL(msg)) {
        lv_subject_set_int(&subjects->show_profile, 0);
      }
    } else if (ps_has_topic(msg, topics::heater::timer_remaining)) {
      if (PS_IS_INT(msg)) {
        lv_subject_set_int(&subjects->timer_remaining, (int32_t)msg->int_val);
        lv_subject_copy_string(&subjects->timer_string, TimeToString((uint32_t)msg->int_val).c_str());
      } else {
        lv_subject_set_int(&subjects->timer_remaining, -1);
        lv_subject_copy_string(&subjects->timer_string, "00:00");
      }
    }
    ps_unref_msg(msg);
  }
}

std::string SubjectManager::TimeToString(uint32_t seconds) {
  char hours[2] = {'\0'};
  // FLOG_INFO("Received timer: %d seconds", seconds);
  if (seconds > 3599) {
    seconds /= 60;  // show HH:MM when over an hour
    hours[0] = 'h';
    hours[1] = '\0';
  }
  uint16_t aa = seconds / 60;
  uint16_t bb = seconds % 60;
  return std::format("{}{:02}:{:02}", hours, aa, bb);
}
}  // namespace toothless
