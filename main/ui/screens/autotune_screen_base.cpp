#include <esp_timer.h>

#include "autotune_screen.hpp"
#include "heater/heater.hpp"
#include "ui/chart_history.hpp"
#include "ui/themes/widget_factories.hpp"

namespace toothless {
//
AutotuneScreen::AutotuneScreen() {
  _screen = ui::CreateScreen();
  _labels = std::make_unique<AutotuneScreenLabels>();
  _subjects = SubjectManager::Instance().subjects;
  _chart = std::make_unique<ChartInfo>();
};

AutotuneScreen::AutotuneScreen(ChartHistory* chart_hist) : AutotuneScreen() {
  _chart = std::make_unique<ChartInfo>();
  _chart->history = chart_hist;
}

AutotuneScreen::~AutotuneScreen() {
  if (_update_timer) {
    lv_timer_set_repeat_count(_update_timer, 0);
    lv_timer_delete(_update_timer);
    _update_timer = nullptr;
  }
  // if (_subscription) {
  //   ps_free_subscriber(_subscription);
  // }
  _chart->history->UnRegister();
}

void AutotuneScreen::UIUpdateTimerCB(lv_timer_t* timer) {
  AutotuneScreen* screen = (AutotuneScreen*)lv_timer_get_user_data(timer);
  if (screen) {
    screen->UpdateChart();
  }
}

esp_err_t AutotuneScreen::Title(lv_obj_t* parent) {
  lv_obj_t* title = ui::CreateTitle(parent, "Autotune");
  if (!title) {
    FLOG_ERROR("Failed to create title label");
    return ESP_ERR_NO_MEM;
  }
  return ESP_OK;
}

esp_err_t AutotuneScreen::Chart(lv_obj_t* parent) {
  lv_obj_t* wrapper = ui::CreateRowContainer(parent);

  lv_obj_set_size(wrapper, lv_pct(100), 0);
  lv_obj_set_flex_grow(wrapper, 1);

  _labels->chart = ui::CreateChart(wrapper, kMaxPoints);
  if (!_labels->chart) {
    FLOG_ERROR("Failed to create chart");
    return ESP_ERR_NO_MEM;
  }
  lv_obj_set_size(_labels->chart, 0, lv_pct(100));
  lv_obj_set_flex_grow(_labels->chart, 1);

  // lv_chart_set_update_mode(_labels->chart, LV_CHART_UPDATE_MODE_SHIFT);
  // lv_chart_set_update_mode(_labels->chart, LV_CHART_UPDATE_MODE_CIRCULAR);
  lv_chart_series_t* temp_series =
      lv_chart_add_series(_labels->chart, lv_palette_main(LV_PALETTE_RED), LV_CHART_AXIS_PRIMARY_Y);
  lv_chart_series_t* target_series =
      lv_chart_add_series(_labels->chart, lv_palette_main(LV_PALETTE_BLUE), LV_CHART_AXIS_PRIMARY_Y);
  lv_chart_set_type(_labels->chart, LV_CHART_TYPE_LINE);
  // Do not display points on the data
  lv_obj_set_style_size(_labels->chart, 0, 0, LV_PART_INDICATOR);

  lv_chart_set_div_line_count(_labels->chart, kYLabelCount, 5);

  _labels->chart_scale_right = ui::CreateChartScale(wrapper, kYLabelCount, false);

  // ChartSetScale();
  lv_chart_set_point_count(_labels->chart, kMaxPoints);  // Keep last 100 points
  _chart->series_map = {{std::string("sensor.temperature.zone"), temp_series},
                        {std::string(topics::heater::target_temperature), target_series}};
  _chart->history->Register(_labels->chart, _chart->series_map);

  return ESP_OK;
}

void AutotuneScreen::UpdateChart() {
  uint32_t starter = esp_timer_get_time();
  static uint32_t last_scale_update = esp_timer_get_time() / 1000;
  static uint32_t last_max;
  uint32_t now = esp_timer_get_time() / 1000;  // milliseconds

  static size_t last_update_index = -1;
  size_t current_index = _chart->history->GetIndex();  // Global head position

  if (current_index != SIZE_MAX && current_index != last_update_index) {
    // Update ALL series together to keep them synchronized
    for (auto& [topic, series_ptr] : _chart->series_map) {
      lv_coord_t latest = _chart->history->GetLatest(topic);
      // if (latest != LV_CHART_POINT_NONE) {
      lv_chart_set_next_value(_labels->chart, series_ptr, latest);
      FLOG_DEBUG("Chart topic %s latest value: %d (at index %d)", topic.c_str(), latest, current_index);
      // }
    }
    last_update_index = current_index;
    if (now - last_scale_update > 1000) {
      int32_t max = _chart->history->GetScale();
      // if (max == last_max) {
      //   FLOG_DEBUG("Chart update took %u us", (uint32_t)(esp_timer_get_time() - starter));
      //   return;
      // }
      last_max = max;
      // _y_axis_labels = _chart_history->YAxisLabels(0, max);
      // for (size_t i = 0; i < kYLabelCount; i++) {
      //   label_pointers[i] = _y_axis_labels.labels[i].c_str();
      // }
      // label_pointers[kYLabelCount] = nullptr;
      lv_chart_set_axis_range(_labels->chart, LV_CHART_AXIS_PRIMARY_Y, 0, (float)max);
      // lv_scale_set_text_src(_labels->chart_scale_right, label_pointers);

      auto labels = _chart->history->YAxisLabelPointers(0, max);  // returns std::array<const char*,kYLabelCount>
      for (size_t i = 0; i < kYLabelCount; i++) _chart->label_pointers[i] = labels[i];
      _chart->label_pointers[kYLabelCount] = nullptr;  // null terminator if LVGL expects it
      lv_scale_set_text_src(_labels->chart_scale_right, _chart->label_pointers.data());
      last_scale_update = now;
    }
  }
  FLOG_DEBUG("Chart update took %u us", (uint32_t)(esp_timer_get_time() - starter));
}

esp_err_t AutotuneScreen::Temperature(lv_obj_t* parent) {
  lv_obj_t* temp_container = ui::CreateRowContainer(parent);
  lv_obj_set_size(temp_container, lv_pct(100), LV_SIZE_CONTENT);  // 60);

  // lv_obj_set_style_border_width(temp_container, 1, 0);
  // lv_obj_set_style_border_color(temp_container, lv_color_hex(0x999900), 0);

  _labels->temp_current =
      ui::CreateLabeledIntUnit(temp_container, "Current", "°C", &_subjects->temperature, "%d", true);
  lv_obj_bind_state_if_not_eq(_labels->temp_current, &_subjects->heater_power, LV_STATE_USER_1, 0);

  _labels->temp_target = ui::CreateLabeledIntUnit(temp_container, "Tune Target", "°C", &_subjects->target, "%d", true);
  _labels->temp_probe = ui::CreateLabeledIntUnit(temp_container, "Probe", "°C", &_subjects->probe, "%d", true);
  // StatusBar(temp_container);

  return ESP_OK;
}

}  // namespace toothless