#include "ui/screens/reflow_screen.hpp"

#include <esp_timer.h>

#include <algorithm>

#include "funlog.h"
#include "heater/heater.hpp"
#include "local_helpers.hpp"
#include "ui/display/display.hpp"
#include "ui/screens/screen_helpers.hpp"
#include "ui/subjects.hpp"
#include "ui/themes/widget_factories.hpp"

extern "C" {
#include <pubsub.h>
}

namespace toothless {

ReflowScreen::ReflowScreen() {
  _labels = std::make_unique<ReflowScreenLabels>();
  _subjects = std::make_unique<Subjects>();
}

ReflowScreen::ReflowScreen(ChartHistory* chart_hist) : ReflowScreen() {
  _chart = std::make_unique<ChartInfo>();
  _chart->history = chart_hist;
}

ReflowScreen::~ReflowScreen() {
  if (_update_timer) {
    lv_timer_set_repeat_count(_update_timer, 0);
    lv_timer_delete(_update_timer);
    _update_timer = nullptr;
  }
  if (_subscription) {
    ps_free_subscriber(_subscription);
  }
  _chart->history->UnRegister();
}

lv_obj_t* ReflowScreen::Create() {
  // esp_log_level_set(FLOG_SHORT_FILENAME, ESP_LOG_DEBUG);
  // _subscription = ps_new_subscriber(10, PS_STRLIST("sensor.temperature.chamber", "heater.target.temperature",
  //  "heater.power", "heater.state", "heater"));

  _screen = ui::CreateScreen();

  _subjects = SubjectManager::Instance().subjects;

  _labels->left = ui::CreateSubScreen(_screen);
  lv_obj_set_size(_labels->left, lv_pct(50), lv_pct(100));

  _labels->right = ui::CreateSubScreen(_screen);
  lv_obj_set_size(_labels->right, lv_pct(50), lv_pct(100));
  lv_obj_set_style_pad_gap(_labels->right, 0, 0);

  // lv_obj_set_style_border_width(_labels->right, 1, 0);
  // lv_obj_set_style_border_color(_labels->right, lv_color_hex(0x999900), 0);

  lv_obj_set_layout(_labels->right, LV_LAYOUT_FLEX);
  lv_obj_set_flex_flow(_labels->right, LV_FLEX_FLOW_COLUMN);

  lv_obj_set_x(_labels->right, lv_pct(50));

  Chart();
  Temperature(_labels->right);
  MidSection(_labels->right);
  _labels->startstop_label = CreateBottomRow(_labels->right);
  lv_label_bind_text(_labels->startstop_label, &_subjects->start_stop, "%s");

  _update_timer = lv_timer_create(UIUpdateTimerCB, kUIUpdateIntervalMs, this);
  PS_PUB_NIL("heater.profile.get");
  return _screen;
}

void ReflowScreen::Loop() {}

void ReflowScreen::UIUpdateTimerCB(lv_timer_t* timer) {
  ReflowScreen* screen = (ReflowScreen*)lv_timer_get_user_data(timer);
  if (screen) {
    screen->UpdateChart();
  }
}

esp_err_t ReflowScreen::UpdateAllDisplays() {
  return ESP_OK;
  FLOG_ERROR("ReflowScreen::UpdateAllDisplays is deprecated, use Subjects instead");
  return ESP_OK;
}

esp_err_t ReflowScreen::Chart() {
  FLOG_ERROR("ReflowScreen::Chart should be centralised");
  lv_obj_t* wrapper = ui::CreateRowContainer(_labels->left);

  lv_obj_set_size(wrapper, lv_pct(100), 0);
  // lv_obj_set_style_min_height(wrapper, 120, 0);
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
  _chart->series_map = {{std::string("sensor.temperature.chamber"), temp_series},
                        {std::string("heater.target.temperature"), target_series}};
  _chart->history->Register(_labels->chart, _chart->series_map);

  return ESP_OK;
}

void ReflowScreen::UpdateChart() {
  uint32_t starter = esp_timer_get_time();
  static uint32_t last_scale_update = esp_timer_get_time() / 1000;
  static uint32_t last_max;
  uint32_t now = esp_timer_get_time() / 1000;  // milliseconds

  static size_t last_update_index = 0;
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

esp_err_t ReflowScreen::Temperature(lv_obj_t* parent) {
  lv_obj_t* temp_container = ui::CreateRowContainer(parent);
  lv_obj_set_size(temp_container, lv_pct(100), LV_SIZE_CONTENT);  // 60);

  // lv_obj_set_style_border_width(temp_container, 1, 0);
  // lv_obj_set_style_border_color(temp_container, lv_color_hex(0x999900), 0);

  _labels->temp_current =
      ui::CreateLabeledIntUnit(temp_container, "Current", "°C", &_subjects->temperature, "%d", true);
  _labels->temp_target = ui::CreateLabeledIntUnit(temp_container, "Target", "°C", &_subjects->target, "%d", true);
  _labels->temp_probe = ui::CreateLabeledIntUnit(temp_container, "Probe", "°C", &_subjects->probe, "%d", true);

  return ESP_OK;
}

esp_err_t ReflowScreen::MidSection(lv_obj_t* parent) {
  lv_obj_t* mid = ui::CreateRowContainer(_labels->right);
  lv_obj_set_height(mid, LV_SIZE_CONTENT);
  lv_obj_set_width(mid, lv_pct(100));
  lv_obj_set_flex_grow(mid, 1);
  lv_obj_set_flex_flow(mid, LV_FLEX_FLOW_ROW_WRAP);
  lv_obj_set_flex_align(mid, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  // lv_obj_set_style_pad_gap(mid, 5, 0);  // Gap between blocks

  // lv_obj_set_style_flex_cross_place(mid, LV_FLEX_ALIGN_CENTER, 0);
  // lv_obj_set_style_flex_main_place(mid, LV_FLEX_ALIGN_CENTER, 0);

  // lv_obj_set_style_border_width(mid, 1, 0);
  // lv_obj_set_style_border_color(mid, lv_color_hex(0x990099), 0);

  {
    _labels->profile = ui::CreateContainer(mid);
    // lv_obj_set_style_border_width(_labels->profile, 1, 0);
    // lv_obj_set_style_border_color(_labels->profile, lv_color_hex(0x990000), 0);

    lv_obj_set_size(_labels->profile, LV_SIZE_CONTENT, LV_SIZE_CONTENT);

    lv_obj_t* profile_label = ui::CreateBodyText(_labels->profile, "No Profile Loaded");
    lv_obj_set_size(profile_label, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_label_bind_text(profile_label, &_subjects->profile, "%s");
    lv_obj_add_flag(_labels->profile, LV_OBJ_FLAG_HIDDEN);
    lv_obj_bind_flag_if_eq(_labels->profile, &_subjects->show_profile, LV_OBJ_FLAG_HIDDEN, 0);

    // lv_obj_set_style_border_width(profile_label, 1, 0);
    // lv_obj_set_style_border_color(profile_label, lv_color_hex(0x009999), 0);
  }
  {
    _labels->stage = ui::CreateContainer(mid);
    ui::CreateSmallText(_labels->stage, LV_SYMBOL_RIGHT);
    lv_obj_t* stage_label = ui::CreateBodyText(_labels->stage, "-");
    lv_label_bind_text(stage_label, &_subjects->stage, "%s");
    lv_obj_add_flag(_labels->stage, LV_OBJ_FLAG_HIDDEN);
    lv_obj_bind_flag_if_eq(_labels->stage, &_subjects->show_stage, LV_OBJ_FLAG_HIDDEN, 0);
  }

  return ESP_OK;
}
}  // namespace toothless
