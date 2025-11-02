#pragma once

#include <lvgl.h>

extern "C" {
#include <pubsub.h>
}

namespace toothless {

class Screen { // Abstract interface
public:
  virtual lv_obj_t *Create() = 0;
  virtual void Loop() = 0;
  ~Screen() { ps_unsubscribe_all(_subscription); };

protected:
  ps_subscriber_t *_subscription;
}; // namespace toothless

} // namespace toothless