// helpers/rolling_average.hpp
#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

// TODO: rolling average
/*
Beelsebob — 20:23Saturday, November 1, 2025 at 20:23
keep a vector of n values initialised to 0 and an average value initialised to 0, each frame, subtract
vector[f%n]/n from the average, put your new value in vector[f % n] where f is the frame number.   And the. Add
vector[f%n]/n to the average. Where n is the number of frames you’re averaging over If you want slightly more
complex code but faster initialisation, initialise all vertor values and the average to your first reading
*/

namespace toothless {

template <typename T, size_t N>
class RollingAverage {
 public:
  void Add(T value) {
    _sum -= _samples[_index];
    _samples[_index] = value;
    _sum += value;
    _index = (_index + 1) % N;
    if (_count < N) ++_count;
  }

  T Get() const { return _count > 0 ? _sum / _count : T{0}; }

  void Reset(T initial = T{0}) {
    _samples.fill(initial);
    _sum = initial * N;
    _index = 0;
    _count = N;
  }

 private:
  std::array<T, N> _samples{};
  T _sum{0};
  size_t _index{0};
  size_t _count{0};
};

}  // namespace toothless