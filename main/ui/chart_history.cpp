#include "chart_history.hpp"

#include <esp_timer.h>

#include <algorithm>
#include <cmath>
#include <format>
#include <unordered_set>

#include "funlog.h"

namespace toothless {

ChartHistory::ChartHistory() { _subscription = ps_new_subscriber(10, PS_STRLIST("health")); }

ChartHistory::ChartHistory(std::string topic) : ChartHistory() { New(topic); }

ChartHistory::~ChartHistory() {
  ps_unsubscribe_all(_subscription);
  ps_free_subscriber(_subscription);
}

void ChartHistory::New(std::string topic) { New(topic, false); }

void ChartHistory::New(std::string topic, bool persist) {
  Series& series = _series[topic];  // Creates if not exists
  series.topic = _series.find(topic)->first.c_str();
  series.persist = persist;
  series.data.fill(std::numeric_limits<float>::quiet_NaN());

  ps_subscribe(_subscription, _series[topic].topic);
  FLOG_INFO("Watching %s", _series[topic].topic);
}

void ChartHistory::Loop() {
  static int64_t last = esp_timer_get_time();
  if (esp_timer_get_time() - last < 250000) {  // TODO: make configurable;
    return;
  }
  last = esp_timer_get_time();
  // Writer contract:
  // 1. Compute slot index from current _count
  // 2. Write all series data into that slot
  // 3. Publish by incrementing _count with release semantics
  // This ensures readers see fully-written slots via acquire on _count.

  size_t idx = _count.load(std::memory_order_relaxed) % kMaxPoints;
  FLOG_DEBUG("ChartHistory Loop at index %d", idx);

  // track which topics we updated this tick
  std::unordered_set<std::string> touched;

  ps_msg_t* msg;
  for ((msg = ps_get(_subscription, 0)); msg != NULL; (msg = ps_get(_subscription, 0))) {
    std::string topic(msg->topic);
    if (!_series.contains(topic)) {
      ps_unref_msg(msg);
      continue;
    }

    if (PS_IS_INT(msg)) {
      float v = msg->int_val / 100.0f;
      _series[topic].data[idx] = v;
      touched.insert(topic);
    } else if (PS_IS_NIL(msg)) {
      _series[topic].data[idx] = std::numeric_limits<float>::quiet_NaN();
      touched.insert(topic);
    }
    ps_unref_msg(msg);
  }

  // For any series that had no message this tick, write NaN to advance its timeline.
  for (auto& kv : _series) {
    const std::string& topic = kv.first;
    Series& s = kv.second;
    if (touched.find(topic) != touched.end()) continue;
    if (s.persist) {
      // copy previous value (if any), preserving last sample; safe if prev is NaN
      size_t prev_idx = (idx == 0) ? (kMaxPoints - 1) : (idx - 1);
      s.data[idx] = s.data[prev_idx];
    } else {
      // advance timeline with an empty/gap value
      s.data[idx] = std::numeric_limits<float>::quiet_NaN();
    }
  }

  _count.fetch_add(1, std::memory_order_release);
}

void ChartHistory::Register(lv_obj_t* chart, std::map<std::string, lv_chart_series_t*> series) {
  _chart = chart;
  for (auto const& [key, val] : series) {
    if (_series.contains(key)) {
      _series[key].chart_series = series[key];
    }
  }
  Replay(chart, series);  // Fill with history
}

void ChartHistory::UnRegister() {
  _chart = nullptr;
  for (auto const& [key, val] : _series) {
    if (val.chart_series) {
      _series[key].chart_series = nullptr;
    }
  }
}

size_t ChartHistory::GetIndex() const {
  size_t count = _count.load(std::memory_order_acquire);
  if (count == 0) return SIZE_MAX;
  return (count - 1) % kMaxPoints;
}

void ChartHistory::Replay(lv_obj_t* chart, std::map<std::string, lv_chart_series_t*> series) {
  auto [start_idx, valid_points] = GetReplayRange();
  if (valid_points == 0) return;

  for (size_t i = 0; i < valid_points; i++) {
    size_t idx = (start_idx + i) % kMaxPoints;
    for (auto& [key, val] : _series) {
      if (!val.chart_series) {
        continue;
      }
      if (std::isnan(val.data[idx])) {
        lv_chart_set_next_value(chart, val.chart_series, LV_CHART_POINT_NONE);
        continue;
      }
      lv_chart_set_next_value(chart, val.chart_series, val.data[idx]);
    }
  }
}

lv_coord_t ChartHistory::GetLatest(const std::string& topic) const {
  // size_t head = _head.load(std::memory_order_acquire);
  size_t idx = GetIndex();

  if (idx == SIZE_MAX) return LV_CHART_POINT_NONE;
  auto it = _series.find(topic);
  if (it == _series.end()) return LV_CHART_POINT_NONE;
  float value = it->second.data[idx % kMaxPoints];
  if (std::isnan(value)) return LV_CHART_POINT_NONE;

  // Round to nearest integer
  // long rounded = lrintf(v);

  // Safe clamping to lv_coord_t bounds
  long minv = static_cast<long>(std::numeric_limits<lv_coord_t>::min());
  long maxv = static_cast<long>(std::numeric_limits<lv_coord_t>::max());
  if (value < minv) value = minv;
  if (value > maxv) value = maxv;

  return static_cast<lv_coord_t>(value);
}

float ChartHistory::MaxValue() const {
  float result = -999.0f;
  for (auto const& [key, val] : _series) {
    auto it = std::max_element(val.data.begin(), val.data.end(), [](float a, float b) {
      if (std::isnan(a)) return true;
      if (std::isnan(b)) return false;
      return a < b;
    });
    // FLOG_INFO("MAX: %.2f for topic %s", (it != val.data.end() ? *it : NAN), key.c_str());
    if (it != val.data.end() && !std::isnan(*it) && *it > result) {
      result = *it;
    }
  }
  return result;
};

std::pair<size_t, size_t> ChartHistory::GetReplayRange() const {
  size_t count = _count.load(std::memory_order_acquire);
  if (count == 0) return {0, 0};

  size_t valid = std::min(count, kMaxPoints);
  size_t start = (count >= kMaxPoints) ? (count % kMaxPoints) : 0;

  return {start, valid};
}

int32_t ChartHistory::GetScale() {
  // TODO: Detect maximum value on the chart atm, we don't want to be smaller than that.
  // Scale to show target + 20% margin
  float max = MaxValue();
  float margin = max * 0.2f;
  int32_t chart_max = (int)(max + margin);
  // Round up to nearest 50 for clean scale
  if (max > 100) {
    chart_max = ((chart_max + 49) / 50) * 50;
  } else {
    chart_max = ((chart_max + 9) / 10) * 10;
  }
  return chart_max;
};

AxisLabels ChartHistory::YAxisLabels(int32_t min_temp, int32_t max_temp) {
  AxisLabels result;
  int32_t temp_range = max_temp - min_temp;
  for (size_t i = 0; i < kYLabelCount; i++) {
    int32_t temp_value = min_temp + (temp_range * (int32_t)i) / (kYLabelCount - 1);
    result.labels[i] = std::format("{}°", temp_value);  // or snprintf into string

    // char temp_str[16];
    // snprintf(temp_str, sizeof(temp_str), "%li°", temp_value);
    // y_left_labels[i] = temp_str;
    // snprintf(_label_strings[i], sizeof(_label_strings[i]), "%li°", temp_value);
    // target[i] = _label_strings[i]; // Point to the string

    // Position relative to chart (0,0 is chart's top-left)
    // int32_t label_y = chart_height - (chart_height * i) / (label_count - 1);
  }
  // y_left_labels[-1] = NULL;
  // target[kYLabelCount] = nullptr; // NULL terminate
  return result;
}

std::array<const char*, kYLabelCount> ChartHistory::YAxisLabelPointers(int32_t min_temp, int32_t max_temp) {
  std::array<const char*, kYLabelCount> result{};
  int32_t temp_range = max_temp - min_temp;
  for (size_t i = 0; i < kYLabelCount; i++) {
    int32_t temp_value = min_temp + (temp_range * (int32_t)i) / (kYLabelCount - 1);
    // Use internal fixed buffers to ensure stable lifetime for C-string pointers
    snprintf(_label_strings[i], sizeof(_label_strings[i]), "%li°", (long)temp_value);
    _label_strings[i][sizeof(_label_strings[i]) - 1] = '\0';
    result[i] = _label_strings[i];
  }
  return result;
}

// int32_t ChartHistory::ChartGetMaxValue() {
//   if (!_labels->chart_series)
//     return 0;

//   // Get the actual number of points currently displayed
//   uint16_t point_count = lv_chart_get_point_count(_labels->chart);
//   int32_t max_value = 0;

//   // Only look at currently visible/stored points
//   int32_t *series = lv_chart_get_series_y_array(_labels->chart, _labels->chart_series);
//   for (uint16_t i = 0; i < point_count; i++) {
//     int32_t value = series[i];
//     // lv_coord_t value = lv_chart_get_point_value_by_id(_labels->chart, _labels->chart_series, i);
//     // Skip "empty" points (LVGL might use LV_CHART_POINT_NONE or negative values for empty)
//     if (value != LV_CHART_POINT_NONE && value > max_value) {
//       max_value = value;
//     }
//   }
//   return max_value;
// }

}  // namespace toothless
