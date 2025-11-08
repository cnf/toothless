#pragma once

#include "ui/screens/screen.hpp"

#include <esp_err.h>
#include <memory>

namespace toothless {

struct HomeScreenLabels : public ScreenLabels {
  lv_obj_t *temperature;
};

class HomeScreen : public Screen {
public:
  HomeScreen();
  ~HomeScreen();
  lv_obj_t *Create();
  void Loop();
  ScreenLabels *GetLabels() override { return _labels.get(); };

private:
  std::unique_ptr<HomeScreenLabels> _labels;
  // ps_subscriber_t *_subscription;
  static void UIUpdateTimerCB(lv_timer_t *timer);
  esp_err_t UpdateAllDisplays();

  esp_err_t Temperature();
  void UpdateTemperatureDisplay(uint32_t temp);

  esp_err_t MidSection();

  esp_err_t BottomRow();

  esp_err_t StartButton(lv_obj_t *container);
  static void StartButtonEventHandler(lv_event_t *e);
  void HandleStartButtonPress();
};
} // namespace toothless