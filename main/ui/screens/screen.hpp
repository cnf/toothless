#pragma once

#include "funlog.h"
#include <lvgl.h>
#include <memory>

extern "C" {
#include <pubsub.h>
}

namespace toothless {

static constexpr uint8_t kUIUpdateIntervalMs = 100; // UI update interval in milliseconds

struct ScreenLabels {
  lv_obj_t *backdrop;
  lv_obj_t *numpad;
  lv_obj_t *numpadtextarea;
  lv_obj_t *confirm_msgbox;
  lv_obj_t *spinbox;
  lv_obj_t *ok_btn;
  bool has_pending = false;
  bool suppress_events = false;
  int32_t pending_value = 0;
};

class Screen { // Abstract interface
public:
  Screen() {};
  virtual ~Screen() {};
  virtual lv_obj_t *Create() = 0;
  virtual void Loop() = 0;
  lv_obj_t *GetScreen() const { return _screen; }
  virtual ScreenLabels *GetLabels() = 0;

protected:
  lv_timer_t *_update_timer;
  ps_subscriber_t *_subscription;
  lv_obj_t *_screen = nullptr;
  std::unique_ptr<ScreenLabels> _labels;

  bool _has_pending = false;
  int32_t _pending_value = 0;
  lv_obj_t *_confirm_msgbox = nullptr;
  // Event guard to avoid re-entrant LVGL event loops when updating controls
  bool _suppress_events = false;
};

} // namespace toothless