#pragma once

#include <esp_err.h>
#include <lvgl.h>

#include <array>
#include <atomic>
#include <cstddef>
#include <limits>
#include <map>
#include <optional>
#include <string>

extern "C" {
#include <pubsub.h>
}

namespace toothless {

static constexpr size_t kMaxPoints = 600;
static constexpr size_t kYLabelCount = 6;
// #define Y_LABEL_COUNT 6

struct AxisLabels {
  std::array<std::string, kYLabelCount> labels;
};

struct Series {
  const char* topic = nullptr;
  lv_chart_series_t* chart_series = nullptr;
  std::array<float, kMaxPoints> data = {std::numeric_limits<float>::quiet_NaN()};
  bool persist = false;
};

/// @brief ChartHistory manages a ring buffer of historical data for multiple series,
///        and provides methods to replay this data into an LVGL chart object.
class ChartHistory {
 public:
  ChartHistory();
  ChartHistory(std::string name, std::string topic);
  ChartHistory(std::string topic);
  ~ChartHistory();
  ChartHistory(const ChartHistory&) = delete;             //<! disable copy constructor because of atomic _head
  ChartHistory& operator=(const ChartHistory&) = delete;  //<! disable copy assignment because of atomic _head

  void New(std::string topic, bool persist);
  void New(std::string topic);
  void Loop();
  // size_t GetHead() { return _head.load(); };

  void Register(lv_obj_t* chart, std::map<std::string, lv_chart_series_t*> series);
  void UnRegister();

  /// @brief Get the ring buffer index of the most recently written sample.
  /// @return Index in [0, kMaxPoints) of the last sample, or SIZE_MAX if no samples yet.
  /// Thread-safe: uses acquire semantics on _count.
  size_t GetIndex() const;

  void Replay(lv_obj_t* chart, std::map<std::string, lv_chart_series_t*> series);

  // std::optional<float> GetLatestValue(const std::string &topic) const;
  lv_coord_t GetLatest(const std::string& topic) const;
  int32_t GetScale();
  AxisLabels YAxisLabels(int32_t min_temp, int32_t max_temp);
  /// @brief Return pointers to stable, null-terminated C strings for Y axis labels.
  /// The returned pointers reference internal storage owned by ChartHistory and
  /// remain valid until the next call that mutates the storage.
  std::array<const char*, kYLabelCount> YAxisLabelPointers(int32_t min_temp, int32_t max_temp);

 private:
  /// @brief Monotonic sample counter (total samples written since start).
  /// Writer: fetch_add(1, memory_order_release) after writing slot.
  /// Readers: load(memory_order_acquire) to compute last valid index.
  /// This is the single authoritative publication point for new samples.
  std::atomic<uint64_t> _count{0};
  ps_subscriber_t* _subscription = nullptr;
  std::map<std::string, Series> _series;
  lv_obj_t* _chart = nullptr;
  char _label_strings[kYLabelCount][16];  // Array of string buffers
  // const char *label_pointers[kYLabelCount + 1]; // Array of pointers + NULL terminator

  float MaxValue() const;

  /// @brief Get the ring buffer range needed to replay the entire history in order.
  /// @return {start_index, valid_count} where start_index is the oldest valid slot,
  ///         and valid_count is the number of samples to replay.
  ///         Returns {0, 0} if no samples exist.
  /// Thread-safe: uses acquire semantics on _count.

  std::pair<size_t, size_t> GetReplayRange() const;

  esp_err_t ChartSetScale();

  int32_t ChartGetMaxValue();
};

}  // namespace toothless