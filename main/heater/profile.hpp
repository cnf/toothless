#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

namespace toothless {

constexpr size_t kMaxstages = 6;

class Profile {
 public:
  enum class Shape { Linear, Smooth };
  struct Stage {
    float start_temp;
    float end_temp;
    uint32_t duration_ms;
    Shape shape = Shape::Linear;
    std::string name;
  };

  struct Sample {
    uint32_t timeMs;
    float temp;
    size_t stage_index;
  };
  Profile(Stage* stages, size_t count) : _stages(stages), _nr_of_stages(count) {};

  size_t GenerateCurve(Sample* buffer, size_t max_samples, uint32_t step_ms = 1000) const;
  std::string CurrentStage(uint32_t elapsed_ms);
  /// @brief Total duration of the profile in milliseconds
  /// @return Total duration in milliseconds
  uint32_t TotalDuration() const;

  float TargetTemp(uint32_t elapsed_ms) const;

 private:
  Stage* _stages;
  size_t _nr_of_stages;
};

}  // namespace toothless