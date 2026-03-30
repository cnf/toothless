#pragma once

#include <esp_err.h>

#include <memory>

#include "ui/chart_history.hpp"
#include "ui/screens/screen.hpp"

namespace toothless {

struct ReflowScreenLabels : public ScreenLabels {
  lv_chart_series_t* chart_series;
  lv_obj_t* chart;
  lv_obj_t* chart_scale_right;
  lv_obj_t* temp_current;
  lv_obj_t* temp_target;
  lv_obj_t* temp_probe;
  lv_obj_t* heater_led;
  lv_obj_t* startstop_label;
  lv_obj_t* stage;
  lv_obj_t* profile;
  lv_obj_t* left;   // TODO: make conditional
  lv_obj_t* right;  // TODO: make conditional
};

// struct ReflowSubjects : public Subjects {
//   lv_subject_t probe_value;
//   lv_subject_t profile;
//   lv_subject_t show_profile;
//   lv_subject_t stage;
//   lv_subject_t show_stage;
// };

// struct ChartInfo {
//   lv_obj_t* chart;
//   uint32_t scale;
//   AxisLabels y_axis_labels;
//   std::array<const char*, kYLabelCount + 1> label_pointers;
//   ChartHistory* history;  // UI owns it
//   std::map<std::string, lv_chart_series_t*> series_map;
// };

class ReflowScreen : public Screen {
 public:
  ReflowScreen();
  ReflowScreen(ChartHistory* chart_hist);
  ~ReflowScreen();
  lv_obj_t* Create();
  void Loop();
  ScreenLabels* GetLabels() override { return _labels.get(); };

 private:
  std::unique_ptr<ReflowScreenLabels> _labels;
  int32_t _target_temp;
  std::unique_ptr<ChartInfo> _chart;

  // AxisLabels _y_axis_labels;
  // // const char* _olabel_pointers[kYLabelCount + 1];  // Array of pointers + NULL terminator
  // std::array<const char*, kYLabelCount + 1> _label_pointers;
  // ChartHistory* _chart_history;  // UI owns it
  // std::map<std::string, lv_chart_series_t*> _chart_series_map;

  static void UIUpdateTimerCB(lv_timer_t* timer);
  esp_err_t UpdateAllDisplays();

  esp_err_t Chart(lv_obj_t* parent);
  void UpdateChart();

  esp_err_t Temperature(lv_obj_t* parent);
  lv_obj_t* TemperatureBlock(lv_obj_t* parent, const char* title, const char* temp);
  void HeaterLED(lv_obj_t* parent);

  void TemperatureUpdateCurrent(int32_t temp);
  void TemperatureUpdateTarget(int32_t temp);
  void TemperatureClearTarget();
  static void TemperatureSetTargetHandler(lv_event_t* e);

  esp_err_t MidSection(lv_obj_t* parent);

  esp_err_t BottomRow();
  esp_err_t StartButton(lv_obj_t* container);
  esp_err_t SettingsButton(lv_obj_t* container);
  esp_err_t StartStopButton(lv_obj_t* container);
  static void ButtonEventHandler(lv_event_t* e);
  void StopButtonPress();

  void StopConfirmation();
  static void ButtonCB(lv_event_t* e);
};
}  // namespace toothless