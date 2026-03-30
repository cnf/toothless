#pragma once

#include <esp_err.h>

#include <memory>

#include "ui/chart_history.hpp"
#include "ui/screens/screen.hpp"

namespace toothless {

struct AutotuneScreenLabels : public ScreenLabels {
  lv_obj_t* chart;
  lv_obj_t* chart_scale_right;
  lv_chart_series_t* chart_series;
  lv_obj_t* temp_current;
  lv_obj_t* temp_target;
  lv_obj_t* temp_probe;
  //   lv_obj_t* set_target;
  //   lv_obj_t* timer;
  //   lv_obj_t* timer_target;
  //   lv_obj_t* heater_led;
  //   lv_obj_t* start_stop_button;
  lv_obj_t* left;   // TODO: make conditional
  lv_obj_t* right;  // TODO: make conditional
};

// struct ChartInfo {
//   lv_obj_t* chart;
//   uint32_t scale;
//   AxisLabels y_axis_labels;
//   std::array<const char*, kYLabelCount + 1> label_pointers;
//   ChartHistory* history;  // UI owns it
//   std::map<std::string, lv_chart_series_t*> series_map;
// };

class AutotuneScreen : public Screen {
 public:
  AutotuneScreen();
  AutotuneScreen(ChartHistory* chart_hist);
  ~AutotuneScreen();
  lv_obj_t* Create();
  void Loop();
  ScreenLabels* GetLabels() override { return _labels.get(); };
  esp_err_t Chart(lv_obj_t* parent);
  void UpdateChart();
  esp_err_t Temperature(lv_obj_t* parent);
  static void UIUpdateTimerCB(lv_timer_t* timer);

  esp_err_t Title(lv_obj_t* parent);

 private:
  std::unique_ptr<AutotuneScreenLabels> _labels;
  std::unique_ptr<ChartInfo> _chart;
};
}  // namespace toothless