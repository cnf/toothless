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
  // ps_subscriber_t *_subscription;
  static void UIUpdateTimerCB(lv_timer_t* timer);
  esp_err_t UpdateAllDisplays();

  esp_err_t Temperature();
  void TemperatureUpdateTarget(int32_t temp);
  void TemperatureClearTarget();
  void TemperatureUpdateCurrent(int32_t temp);
  void TimerUpdate(uint32_t seconds);
  void TimerClear();
  void UpdateTemperatureDisplay(uint32_t temp);

  void MainSection();

  void CreateTemperature(lv_obj_t* parent);

  void CreateTimer(lv_obj_t* parent);

  static void TimerHandler(lv_event_t* e);

  static void TargetHandler(lv_event_t* e);

  void HeaterLED(lv_obj_t* parent);

  esp_err_t BottomRow();

  esp_err_t StartButton(lv_obj_t* container);
  static void StartButtonEventHandler(lv_event_t* e);
  void HandleStartButtonPress();
};
}  // namespace toothless