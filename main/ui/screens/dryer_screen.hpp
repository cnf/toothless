#pragma once

#include <esp_err.h>

#include <memory>

#include "ui/screens/screen.hpp"

namespace toothless {

struct DryerScreenLabels : public ScreenLabels {
  lv_obj_t* temperature_current;
  lv_obj_t* temperature_target;
  lv_obj_t* set_target;
  lv_obj_t* timer;
  lv_obj_t* timer_target;
  lv_obj_t* heater_led;
  lv_obj_t* start_stop_button;
  lv_obj_t* left;   // TODO: make conditional
  lv_obj_t* right;  // TODO: make conditional
};

class DryerScreen : public Screen {
 public:
  DryerScreen();
  ~DryerScreen();
  lv_obj_t* Create();
  void Loop();
  ScreenLabels* GetLabels() override { return _labels.get(); };

 private:
  std::unique_ptr<DryerScreenLabels> _labels;

  void CreateTemperature(lv_obj_t* parent);

  void CreateTimerTargetRow(lv_obj_t* parent);

  lv_obj_t* CreateTimer(lv_obj_t* parent);

  static void TimerHandler(lv_event_t* e);

  lv_obj_t* CreateTarget(lv_obj_t* parent);

  static void TargetHandler(lv_event_t* e);

  void HeaterLED(lv_obj_t* parent);
};
}  // namespace toothless