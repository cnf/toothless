#include "profile.hpp"

#include <cmath>
#include <vector>

namespace toothless {

float Profile::TargetTemp(uint32_t elapsedMs) const {
  if (_nr_of_stages == 0) return 0.0f;

  uint32_t t = 0;
  for (size_t i = 0; i < _nr_of_stages; ++i) {
    const auto& s = _stages[i];
    if (elapsedMs < t + s.duration_ms) {
      float frac = float(elapsedMs - t) / float(s.duration_ms);
      if (s.shape == Shape::Smooth)
        // cosine smooth step interpolation
        frac = (1 - std::cos(frac * 3.1415926f)) * 0.5f;

      return s.start_temp + frac * (s.end_temp - s.start_temp);
    }
    t += s.duration_ms;
  }

  return _stages[_nr_of_stages - 1].end_temp;
}

uint32_t Profile::TotalDuration() const {
  uint32_t sum = 0;
  for (size_t i = 0; i < _nr_of_stages; ++i) sum += _stages[i].duration_ms;
  return sum;
}

// size_t Profile::GenerateCurve(Sample* buffer, size_t max_samples, uint32_t step_ms) const {
//   if (!buffer || _nr_of_stages == 0) return 0;

//   const uint32_t total = TotalDuration();
//   size_t idx = 0;

//   for (uint32_t t = 0; t <= total && idx < max_samples; t += step_ms) {
//     buffer[idx++] = {t, TargetTemp(t)};
//   }

//   return idx;
// }

size_t Profile::GenerateCurve(Sample* buffer, size_t max_samples, uint32_t step_ms) const {
  if (!buffer || _nr_of_stages == 0) return 0;

  size_t idx = 0;
  uint32_t time_acc = 0;

  for (size_t stage = 0; stage < _nr_of_stages; ++stage) {
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

  // Ensure final point equals last stage end
  if (idx < max_samples) buffer[idx++] = {time_acc, _stages[_nr_of_stages - 1].end_temp, _nr_of_stages - 1};

  return idx;
}

std::string Profile::CurrentStage(uint32_t elapsed_ms) {
  uint32_t t = 0;
  for (size_t i = 0; i < _nr_of_stages; ++i) {
    if (elapsed_ms < t + _stages[i].duration_ms) return _stages[i].name;
    t += _stages[i].duration_ms;
  }
  return _stages[_nr_of_stages - 1].name;
}

}  // namespace toothless