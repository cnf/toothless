#include "ui/screens/reflow_screen.hpp"

#include <esp_timer.h>

#include <algorithm>

#include "funlog.h"
#include "heater/heater.hpp"
#include "ui/display/display.hpp"
#include "ui/screens/screen_helpers.hpp"
#include "ui/subjects.hpp"
#include "ui/themes/style_registry.hpp"
#include "ui/themes/widget_factories.hpp"

extern "C" {
#include <pubsub.h>
}

namespace toothless {

ReflowScreen::ReflowScreen() { _labels = std::make_unique<ReflowScreenLabels>(); }

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
  esp_log_level_set(FLOG_SHORT_FILENAME, ESP_LOG_DEBUG);
  _subscription = ps_new_subscriber(
      10, PS_STRLIST("sensor.temperature.zone", "heater.target.temperature", "heater.power", "heater.state", "heater"));

  _screen = ui::CreateScreen();
  lv_obj_set_layout(_screen, LV_LAYOUT_FLEX);          // Set screen to vertical flex layout
  lv_obj_set_flex_flow(_screen, LV_FLEX_FLOW_COLUMN);  // Vertical stacking

  _subjects = SubjectManager::Instance().subjects;

  Temperature(_screen);
  Chart();
  MidSection(_screen);
  _labels->startstop_label = CreateBottomRow(_screen);

  _update_timer = lv_timer_create(UIUpdateTimerCB, kUIUpdateIntervalMs, this);
  return _screen;
}

void ReflowScreen::Loop() {}

void ReflowScreen::UIUpdateTimerCB(lv_timer_t* timer) {
  ReflowScreen* screen = (ReflowScreen*)lv_timer_get_user_data(timer);
  if (screen) {
    screen->UpdateAllDisplays();
    screen->UpdateChart();
  }
}

esp_err_t ReflowScreen::UpdateAllDisplays() { return ESP_OK; }

esp_err_t ReflowScreen::Chart() {
  lv_obj_t* wrapper = ui::CreateRowContainer(_screen);

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

  // lv_chart_set_div_line_count(_labels->chart, kYLabelCount, 5);

  // Scale
  _labels->chart_scale_right = lv_scale_create(wrapper);
  lv_scale_set_mode(_labels->chart_scale_right, LV_SCALE_MODE_VERTICAL_RIGHT);
  lv_obj_set_size(_labels->chart_scale_right, 30, lv_pct(100));
  lv_obj_set_flex_grow(_labels->chart_scale_right, 0);  // Don't grow
  lv_scale_set_total_tick_count(_labels->chart_scale_right, kYLabelCount);
  lv_scale_set_major_tick_every(_labels->chart_scale_right, 1);
  // TODO: see of this needs dynamic calc for different screen
  // lv_obj_set_style_pad_ver(_labels->chart_scale_right, lv_chart_get_first_point_center_offset(_labels->chart), 0);
  lv_obj_set_style_pad_ver(_labels->chart_scale_right, 10, 0);  // Fixed 10px padding
  lv_obj_set_style_text_font(_labels->chart_scale_right, &lv_font_montserrat_12, 0);
  // lv_obj_add_flag(_labels->chart_scale_right, LV_OBJ_FLAG_HIDDEN);

  // ChartSetScale();
  lv_chart_set_point_count(_labels->chart, kMaxPoints);  // Keep last 100 points
  _chart->series_map = {{std::string("sensor.temperature.zone"), temp_series},
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
      if (latest != LV_CHART_POINT_NONE) {
        lv_chart_set_next_value(_labels->chart, series_ptr, latest);
        FLOG_TRACE("Chart topic %s latest value: %d (at index %d)", topic.c_str(), latest, current_index);
      }
    }
    last_update_index = current_index;
    if (now - last_scale_update > 1000) {
      int32_t max = _chart->history->GetScale();
      // if (max == last_max) {
      //   FLOG_TRACE("Chart update took %u us", (uint32_t)(esp_timer_get_time() - starter));
      //   FLOG_DEBUG("Chart scale unchanged at %d, skipping update", max);
      //   return;
      // }
      // last_max = max;
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
      FLOG_DEBUG("Chart scale updated to max %d", max);
    }
  }
  FLOG_TRACE("Chart update took %u us", (uint32_t)(esp_timer_get_time() - starter));
}

esp_err_t ReflowScreen::Temperature(lv_obj_t* parent) {
  static size_t height = lv_display_get_vertical_resolution(NULL) * 0.2;
  height = std::max<size_t>(height, 60);
  lv_obj_t* temp_container = ui::CreateRowContainer(parent);
  lv_obj_set_size(temp_container, lv_pct(100), height);
  lv_obj_set_style_pad_gap(temp_container, 10, 0);  // Gap between temp blocks
  lv_obj_set_flex_align(temp_container, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);

  _labels->temp_current =
      ui::CreateLabeledIntUnit(temp_container, "Current", "°C", &_subjects->temperature, "%d", true);

  // HeaterLED(temp_container);
  _labels->temp_target = ui::CreateLabeledIntUnit(temp_container, "Target", "°C", &_subjects->target, "%d", true);
  _labels->temp_probe = ui::CreateLabeledIntUnit(temp_container, "Probe", "°C", &_subjects->probe, "%d", true);
  return ESP_OK;
}

lv_obj_t* ReflowScreen::TemperatureBlock(lv_obj_t* parent, const char* title, const char* temp) {
  lv_obj_t* obj;
  lv_obj_t* temperature_obj = ui::CreateColumnContainer(parent);
  lv_obj_set_style_border_width(temperature_obj, 3, 0);
  lv_obj_set_style_border_color(temperature_obj, lv_color_hex(0x220099), 0);

  lv_obj_set_flex_grow(temperature_obj, 1);
  lv_obj_set_style_pad_gap(temperature_obj, 2, 0);
  lv_obj_set_flex_align(temperature_obj, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

  // Title
  lv_obj_t* cur_title_label = ui::CreateSmallText(temperature_obj, title);

  lv_obj_t* taco = ui::CreateRowContainer(temperature_obj);

  // Temperature
  obj = ui::CreateValueSmall(taco, NULL, temp);
  // ui::CreateUnitLabel(taco, "°C");

  return obj;
};

void ReflowScreen::HeaterLED(lv_obj_t* parent) {
  // FIXME: Make this use an observer
  lv_obj_t* led_cell = ui::CreateColumnContainer(parent);
  lv_obj_set_size(led_cell, 0, lv_pct(100));
  lv_obj_set_flex_grow(led_cell, 0);
  lv_obj_set_style_min_width(led_cell, 30, 0);

  lv_obj_set_flex_align(led_cell, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

  _labels->heater_led = ui::CreateLEDIndicator(led_cell, false);
  // lv_obj_align(_labels->heater_led, LV_ALIGN_CENTER, 0, 0);
  // lv_obj_set_flex_align(_labels->heater_led, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

  lv_led_set_color(_labels->heater_led, lv_palette_main(LV_PALETTE_RED));
}

void ReflowScreen::TemperatureUpdateCurrent(int32_t temp) {
  FLOG_VERBOSE("Received temperature: %d", temp);
  float temperature = temp / 100.0f;
  char temp_str[16];

  float clamped_temp = std::clamp(temperature, -999.99f, 9999.99f);
  snprintf(temp_str, sizeof(temp_str), "%.0f", clamped_temp);
  lv_label_set_text(_labels->temp_current, temp_str);
}

void ReflowScreen::TemperatureUpdateTarget(int32_t temp) {
  char temp_str[16];
  int32_t clamped_temp = std::clamp<int32_t>(temp / 100, int32_t(-99), int32_t(999));
  snprintf(temp_str, sizeof(temp_str), "%li", clamped_temp);
  lv_label_set_text(_labels->temp_target, temp_str);
}

void ReflowScreen::TemperatureClearTarget() { lv_label_set_text(_labels->temp_target, "-100"); }

esp_err_t ReflowScreen::MidSection(lv_obj_t* parent) {
  lv_obj_t* mid = ui::CreateRowContainer(parent);
  lv_obj_set_height(mid, LV_SIZE_CONTENT);
  lv_obj_set_width(mid, lv_pct(100));

  lv_obj_set_style_min_height(mid, 0, 0);  // important: min-height 0
  lv_obj_set_style_pad_all(mid, 0, 0);     // no padding
  lv_obj_set_style_pad_row(mid, 0, 0);

  lv_obj_set_style_pad_gap(mid, 10, 0);  // Gap between blocks

  // lv_obj_set_style_pad_col(mid, 0, 0);
  // _labels->profile = ui::CreateRowContainer(mid);
  _labels->profile = ui::CreateCard(mid);
  lv_obj_set_height(_labels->profile, 50);
  lv_obj_set_width(_labels->profile, lv_pct(50));
  lv_obj_set_flex_grow(_labels->profile, 1);
  // lv_obj_set_flex_align(_labels->profile, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

  lv_obj_t* title = ui::CreateSmallText(_labels->profile, "Profile: ");

  lv_obj_t* val = ui::CreateSmallText(_labels->profile, "");
  // lv_obj_t* val = ui::CreateBodyText(_labels->profile, "");
  lv_obj_add_flag(_labels->profile, LV_OBJ_FLAG_HIDDEN);

  // _labels->stage = ui::CreateRowContainer(mid);
  _labels->stage = ui::CreateCard(mid);
  lv_obj_set_height(_labels->stage, 50);
  lv_obj_set_width(_labels->stage, lv_pct(50));
  lv_obj_set_flex_grow(_labels->stage, 1);
  // lv_obj_set_flex_align(_labels->stage, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

  ui::CreateSmallText(_labels->stage, "Stage: ");

  ui::CreateSmallText(_labels->stage, "");
  lv_obj_add_flag(_labels->stage, LV_OBJ_FLAG_HIDDEN);

  return ESP_OK;
}

esp_err_t ReflowScreen::BottomRow() {
  static size_t height = lv_display_get_vertical_resolution(NULL) * 0;

  lv_obj_t* wrapper = ui::CreateRowContainer(_screen);

  // StartStopButton(temp_container);
  _labels->startstop_label = CreateStartStopButton(wrapper);
  lv_label_set_text(_labels->startstop_label, "Poop");
  CreateModeButton(wrapper);
  CreateSettingsButton(wrapper);
  // StopButton(temp_container);
  // SettingsButton(temp_container);
  return ESP_OK;
}

void ReflowScreen::ButtonCB(lv_event_t* e) {
  lv_obj_t* button = (lv_obj_t*)lv_event_get_target(e);
  lv_obj_t* backdrop = (lv_obj_t*)lv_event_get_user_data(e);

  // Get button text to determine action
  lv_obj_t* label = lv_obj_get_child(button, 0);  // Button's label
  const char* text = lv_label_get_text(label);

  if (lv_strcmp(text, "Stop!") == 0) {
    FLOG_DEBUG("Stop confirmed!");
    PS_PUB_INT("heater.state.set", heater::kStateOff);

    // PS_PUB_NIL("ui.action.stop");
    // return;
  } else if (lv_strcmp(text, "Cancel") == 0) {
  } else if (lv_strcmp(text, LV_SYMBOL_SETTINGS) == 0) {
    PS_PUB_NIL("ui.action.settings");
  }
  if (backdrop) {
    lv_obj_delete_async(backdrop);
  }
}

}  // namespace toothless
