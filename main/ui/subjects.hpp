#pragma once

#include <lvgl.h>

#include <memory>

extern "C" {
#include <pubsub.h>
}

struct Subjects {
  lv_subject_t temperature;
  lv_subject_t target;
  lv_subject_t probe;
  lv_subject_t heater_power;
  lv_subject_t heater_state;
  lv_subject_t start_stop;
  lv_subject_t profile;
  lv_subject_t show_profile;
  lv_subject_t stage;
  lv_subject_t show_stage;
  lv_subject_t timer_string;
  lv_subject_t timer_remaining;
};

namespace toothless {

class SubjectManager {
 public:
  SubjectManager();
  SubjectManager(const SubjectManager&) = delete;
  SubjectManager& operator=(const SubjectManager&) = delete;
  ~SubjectManager();
  static SubjectManager& Instance() {
    static SubjectManager instance;
    return instance;
  }
  void Init();
  void Loop();

  static std::string TimeToString(uint32_t seconds);

  std::shared_ptr<Subjects> subjects;

 private:
  ps_subscriber_t* _subscription;
  // SubjectManager() = default;
};
}  // namespace toothless