#pragma once

#include <memory>

namespace helper {
template <typename T>
inline void* Box(std::shared_ptr<T> sp) {
  return new std::shared_ptr<T>(std::move(sp));
}

template <typename T>
inline std::shared_ptr<T> Unbox(void* data) {
  auto* box = static_cast<std::shared_ptr<T>*>(data);
  auto sp = std::move(*box);
  delete box;
  return sp;
}

}  // namespace helper