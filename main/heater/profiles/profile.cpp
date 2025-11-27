#include "profile.hpp"

#include <cmath>
#include <sstream>
#include <vector>

namespace toothless {

// Constructor from array (backwards compatible)
Profile::Profile(const std::string& name, const Stage* stages, size_t count) : _name(name), _stages{}, _stage_count(0) {
  if (count > kMaxStages) count = kMaxStages;
  for (size_t i = 0; i < count; ++i) {
    _stages[i] = stages[i];
  }
  _stage_count = count;
}

// Constructor from vector (modern approach)
Profile::Profile(const std::string& name, const std::vector<Stage>& stages) : _name(name), _stages{}, _stage_count(0) {
  size_t count = stages.size();
  if (count > kMaxStages) count = kMaxStages;
  for (size_t i = 0; i < count; ++i) {
    _stages[i] = stages[i];
  }
  _stage_count = count;
}

esp_err_t Profile::AddStage(const Stage& stage) {
  if (_stage_count >= kMaxStages) {
    return ESP_ERR_NO_MEM;
  }
  _stages[_stage_count++] = stage;
  return ESP_OK;
}

esp_err_t Profile::SaveStage(size_t index, const Stage& stage) { return esp_err_t(); }

esp_err_t Profile::RemoveStage(size_t index) {
  if (index >= _stage_count) {
    return ESP_ERR_INVALID_ARG;
  }
  // Shift all stages after index down by one
  for (size_t i = index; i < _stage_count - 1; ++i) {
    _stages[i] = _stages[i + 1];
  }
  _stage_count--;
  return ESP_OK;
}

const Profile::Stage* Profile::GetStage(size_t index) const {
  if (index >= _stage_count) {
    return nullptr;
  }
  return &_stages[index];
}

Profile::Stage* Profile::GetStage(size_t index) {
  // forward to const variant and cast away const for non-const callers
  return const_cast<Profile::Stage*>(static_cast<const Profile*>(this)->GetStage(index));
}

std::string Profile::ToJSON() const {
  std::ostringstream json;
  json << "{\n";
  json << "  \"name\": \"" << _name << "\",\n";
  json << "  \"stages\": [\n";

  for (size_t i = 0; i < _stage_count; ++i) {
    const auto& s = _stages[i];
    json << "    {\n";
    json << "      \"start_temp\": " << s.start_temp << ",\n";
    json << "      \"end_temp\": " << s.end_temp << ",\n";
    json << "      \"duration_ms\": " << s.duration_ms << ",\n";
    json << "      \"shape\": \"" << (s.shape == Shape::Smooth ? "smooth" : "linear") << "\",\n";
    json << "      \"name\": \"" << s.name << "\"\n";
    json << "    }";
    if (i < _stage_count - 1) json << ",";
    json << "\n";
  }

  json << "  ]\n";
  json << "}";

  return json.str();
}

std::shared_ptr<Profile> Profile::FromJSON(const std::string& json) {
  // Simple manual JSON parser for embedded systems
  // This is a minimal implementation - you could use a proper JSON library later

  auto profile = std::make_shared<Profile>();

  // Extract name
  size_t name_start = json.find("\"name\"");
  if (name_start != std::string::npos) {
    name_start = json.find("\"", name_start + 7);
    size_t name_end = json.find("\"", name_start + 1);
    if (name_start != std::string::npos && name_end != std::string::npos) {
      profile->_name = json.substr(name_start + 1, name_end - name_start - 1);
    }
  }

  // Parse stages array
  size_t pos = json.find("\"stages\"");
  if (pos == std::string::npos) return nullptr;

  pos = json.find("[", pos);
  if (pos == std::string::npos) return nullptr;

  size_t stage_idx = 0;
  while (stage_idx < kMaxStages) {
    pos = json.find("{", pos + 1);
    if (pos == std::string::npos) break;

    size_t stage_end = json.find("}", pos);
    if (stage_end == std::string::npos) break;

    std::string stage_json = json.substr(pos, stage_end - pos);
    Stage stage;

    // Parse start_temp
    size_t temp_pos = stage_json.find("\"start_temp\"");
    if (temp_pos != std::string::npos) {
      temp_pos = stage_json.find(":", temp_pos);
      stage.start_temp = std::stof(stage_json.substr(temp_pos + 1));
    }

    // Parse end_temp
    temp_pos = stage_json.find("\"end_temp\"");
    if (temp_pos != std::string::npos) {
      temp_pos = stage_json.find(":", temp_pos);
      stage.end_temp = std::stof(stage_json.substr(temp_pos + 1));
    }

    // Parse duration_ms
    temp_pos = stage_json.find("\"duration_ms\"");
    if (temp_pos != std::string::npos) {
      temp_pos = stage_json.find(":", temp_pos);
      stage.duration_ms = std::stoul(stage_json.substr(temp_pos + 1));
    }

    // Parse shape
    temp_pos = stage_json.find("\"shape\"");
    if (temp_pos != std::string::npos) {
      temp_pos = stage_json.find("\"", temp_pos + 7);
      size_t shape_end = stage_json.find("\"", temp_pos + 1);
      std::string shape_str = stage_json.substr(temp_pos + 1, shape_end - temp_pos - 1);
      stage.shape = (shape_str == "smooth") ? Shape::Smooth : Shape::Linear;
    }

    // Parse name
    temp_pos = stage_json.find("\"name\"");
    if (temp_pos != std::string::npos) {
      temp_pos = stage_json.find("\"", temp_pos + 6);
      size_t name_end = stage_json.find("\"", temp_pos + 1);
      stage.name = stage_json.substr(temp_pos + 1, name_end - temp_pos - 1);
    }

    profile->_stages[stage_idx++] = stage;
    pos = stage_end;
  }

  profile->_stage_count = stage_idx;
  return profile;
}

float Profile::TargetTemp(uint32_t elapsed_ms) const {
  if (_stage_count == 0) return 0.0f;

  uint32_t t = 0;
  for (size_t i = 0; i < _stage_count; ++i) {
    const auto& s = _stages[i];
    if (elapsed_ms < t + s.duration_ms) {
      float frac = float(elapsed_ms - t) / float(s.duration_ms);
      if (s.shape == Shape::Smooth) frac = (1 - std::cos(frac * 3.1415926f)) * 0.5f;

      return s.start_temp + frac * (s.end_temp - s.start_temp);
    }
    t += s.duration_ms;
  }

  return _stages[_stage_count - 1].end_temp;
}

uint32_t Profile::TotalDuration() const {
  uint32_t sum = 0;
  for (size_t i = 0; i < _stage_count; ++i) sum += _stages[i].duration_ms;
  return sum;
}

size_t Profile::GenerateCurve(Sample* buffer, size_t max_samples, uint32_t step_ms) const {
  if (!buffer || _stage_count == 0) return 0;

  size_t idx = 0;
  uint32_t time_acc = 0;

  for (size_t stage = 0; stage < _stage_count; ++stage) {
    const auto& s = _stages[stage];
    for (uint32_t t = 0; t < s.duration_ms && idx < max_samples; t += step_ms) {
      uint32_t global_time = time_acc + t;
      float frac = float(t) / float(s.duration_ms);
      if (s.shape == Shape::Smooth) frac = (1 - std::cos(frac * 3.1415926f)) * 0.5f;

      float temp = s.start_temp + frac * (s.end_temp - s.start_temp);
      buffer[idx++] = {global_time, temp, stage};
    }
    time_acc += s.duration_ms;
  }

  if (idx < max_samples) buffer[idx++] = {time_acc, _stages[_stage_count - 1].end_temp, _stage_count - 1};

  return idx;
}

std::string Profile::CurrentStage(uint32_t elapsed_ms) const {
  uint32_t t = 0;
  for (size_t i = 0; i < _stage_count; ++i) {
    if (elapsed_ms < t + _stages[i].duration_ms) return _stages[i].name;
    t += _stages[i].duration_ms;
  }
  return _stages[_stage_count - 1].name;
}

}  // namespace toothless