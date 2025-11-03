#pragma once

#include "ui/screens/screen.hpp"

#include <esp_err.h>
#include <memory>

namespace toothless {

struct RunningScreenLabels {
  lv_obj_t *temp_current;
  lv_obj_t *temp_target;
  lv_obj_t *chart;
  lv_chart_series_t *chart_series;
  lv_obj_t *chart_scale_right;
};

class RunningScreen : public Screen {
public:
  RunningScreen();
  ~RunningScreen();
  lv_obj_t *Create();
  void Loop();

private:
  lv_obj_t *_screen = nullptr;
  std::unique_ptr<RunningScreenLabels> _labels;
  int32_t _target_temp;
  static constexpr int Y_LABEL_COUNT = 6;
  char label_strings[Y_LABEL_COUNT][16];         // Array of string buffers
  const char *label_pointers[Y_LABEL_COUNT + 1]; // Array of pointers + NULL terminator

  static void UIUpdateTimerCB(lv_timer_t *timer);
  esp_err_t UpdateAllDisplays();

  esp_err_t Chart();
  esp_err_t ChartSetScale();
  esp_err_t ChartSetScale(int32_t scale);
  void ChartYAxisLabels(int32_t min_temp, int32_t max_temp);
  int32_t ChartGetMaxValue();

  esp_err_t Temperature();
  lv_obj_t *TemperatureBlock(lv_obj_t *parent, const char *title, const char *temp);

  void TemperatureUpdateCurrent(int32_t temp);
  void TemperatureUpdateTarget(int32_t temp);

  esp_err_t MidSection();

  esp_err_t TemperatureSlider();
  static void TemperatureSliderHandler(lv_event_t *e);

  esp_err_t BottomRow();
  esp_err_t StopButton(lv_obj_t *container);
  static void StopButtonEventHandler(lv_event_t *e);
  void StopButtonPress();

  void StopConfirmation();
  static void ButtonCB(lv_event_t *e);
};
} // namespace toothless