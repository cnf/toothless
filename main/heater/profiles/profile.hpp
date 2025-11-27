#pragma once

#include <esp_err.h>

#include <array>
#include <memory>
#include <string>
#include <vector>

namespace toothless {

constexpr size_t kMaxStages = 8;  // More flexibility

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

  /// @brief Default constructor
  Profile() : _name("Unnamed"), _stages{}, _stage_count(0) {}

  /// @brief Create from array (backwards compatible)
  Profile(const std::string& name, const Stage* stages, size_t count);

  /// @brief Create from vector (modern approach)
  Profile(const std::string& name, const std::vector<Stage>& stages);

  /// @brief Get stage at index
  /// @param index Index of stage to get
  /// @return Pointer to stage, or nullptr if index invalid
  const Stage* GetStage(size_t index) const;
  Profile::Stage* GetStage(size_t index);

  /// @brief Add a stage
  esp_err_t AddStage(const Stage& stage);

  /// @brief Save stage at index
  /// @param index
  /// @param stage
  /// @return
  esp_err_t SaveStage(size_t index, const Stage& stage);

  /// @brief Remove stage at index
  esp_err_t RemoveStage(size_t index);

  /// @brief Get stage count
  size_t StageCount() const { return _stage_count; }

  /// @brief Get profile name
  std::string Name() const { return _name; }

  /// @brief Set profile name
  void SetName(const std::string& name) { _name = name; }

  /// @brief Serialize to JSON string for storage
  std::string ToJSON() const;

  /// @brief Deserialize from JSON string
  static std::shared_ptr<Profile> FromJSON(const std::string& json);

  // Existing methods...
  size_t GenerateCurve(Sample* buffer, size_t max_samples, uint32_t step_ms = 1000) const;
  std::string CurrentStage(uint32_t elapsed_ms) const;
  uint32_t TotalDuration() const;
  float TargetTemp(uint32_t elapsed_ms) const;

 private:
  std::string _name;
  std::array<Stage, kMaxStages> _stages;  // BUG: should be a shared_ptr
  size_t _stage_count;
};

}  // namespace toothless