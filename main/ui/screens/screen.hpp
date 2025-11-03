#pragma once

#include "funlog.h"
#include <lvgl.h>

extern "C" {
#include <pubsub.h>
}

namespace toothless {

static constexpr uint8_t kUIUpdateIntervalMs = 250; // UI update interval in milliseconds

class Screen { // Abstract interface
public:
  Screen() {};
  virtual ~Screen() {};
  virtual lv_obj_t *Create() = 0;
  virtual void Loop() = 0;
  // virtual ~Screen() {
  //   FLOG_DEBUG("Cleaning up Screen");
  //   if (_subscription) {
  //     ps_free_subscriber(_subscription);
  //     _subscription = nullptr;
  //   }
  //   if (_update_timer) {
  //     lv_timer_delete(_update_timer);
  //     _update_timer = nullptr;
  //   };
  // }

protected:
  lv_timer_t *_update_timer;
  ps_subscriber_t *_subscription;
}; // namespace toothless

} // namespace toothless