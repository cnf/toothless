#pragma once
/*
#include <array>
#include <cstring>

// template <std::size_t ArraySize>
// struct CompileTimeString {
//   char data[ArraySize];

//   constexpr CompileTimeString() noexcept : data{} {}

//   constexpr CompileTimeString(const char (&literal)[ArraySize]) noexcept { std::ranges::copy(literal, data); }

//   template <std::size_t OtherSize>
//   constexpr auto operator+(const CompileTimeString<OtherSize>& other) const noexcept {
//     CompileTimeString<ArraySize + OtherSize - 1> result;
//     std::ranges::copy(data, result.data);
//     std::ranges::copy(other.data, result.data + ArraySize - 1);
//     return result;
//   }

//   constexpr const char* c_str() const { return data; }
// };

// template <CompileTimeString Str>
// constexpr auto operator""_cts() {
//   return Str;
// }

namespace topic {
static constexpr const int kMaxTopicLength = 64;
inline constexpr const char root[] = "";
namespace verb {
// String literals for verb
inline constexpr const char get[] = "get";
inline constexpr const char set[] = "set";
inline constexpr const char erase[] = "erase";
inline constexpr const char add[] = "add";
inline constexpr const char remove[] = "remove";
inline constexpr const char toggle[] = "toggle";
}  // namespace verb

// Zero-overhead compile-time join for 2 strings
// #define TOPIC_JOIN(a, b) a "." b

// // Runtime join for variable parts (uses small stack buffer)
// inline const char* join(const char* a, const char* b, char* buf, size_t buf_size) {
//   snprintf(buf, buf_size, "%s.%s", a, b);
//   return buf;
// }

// // Helper: concat with automatic "." separator
// template <std::size_t N1, std::size_t N2>
// constexpr auto concat(const CompileTimeString<N1>& a, const CompileTimeString<N2>& b) {
//   return a + "."_cts + b;
// }

// Compile-time string concatenation
// topics/topics.hpp - add this overload:

// template <size_t N>
// constexpr std::array<char, N> concat(const char (&a)[N]) {
//   std::array<char, N> result{};
//   size_t i = 0;
//   for (size_t j = 0; j < N - 1; ++j) result[i++] = a[j];
//   // result[i++] = '.';
//   // for (size_t j = 0; j < N2 - 1; ++j) result[i++] = b[j];
//   result[i] = '\0';
//   return result;
// }

// concat: returns std::array
template <size_t N1, size_t N2>
constexpr std::array<char, N1 + N2> concat(const char (&a)[N1], const char (&b)[N2]) {
  std::array<char, N1 + N2> result{};
  size_t i = 0;
  for (size_t j = 0; j < N1 - 1; ++j) result[i++] = a[j];
  result[i++] = '.';
  for (size_t j = 0; j < N2 - 1; ++j) result[i++] = b[j];
  result[i] = '\0';
  return result;  // Fixed: removed extra 'r'
}

// Overload for std::array + literal
template <size_t N1, size_t N2>
constexpr std::array<char, N1 + N2> concat(const std::array<char, N1>& a, const char (&b)[N2]) {
  std::array<char, N1 + N2> result{};
  size_t i = 0;
  for (size_t j = 0; j < N1 && a[j] != '\0'; ++j) result[i++] = a[j];
  result[i++] = '.';
  for (size_t j = 0; j < N2 - 1; ++j) result[i++] = b[j];
  result[i] = '\0';
  return result;
}

// compile: returns const char* (one-liner usage)
template <size_t N1, size_t N2>
constexpr const char* compile(const char (&a)[N1], const char (&b)[N2]) {
  static constexpr auto arr = concat(a, b);
  return arr.data();
}

// // Single-arg compile: just return the literal directly
// template <size_t N>
// constexpr const char* compile(const char (&a)[N]) {
//   return a;  // No .data() - it's already a literal
// }

// with_verb: must use static to keep array alive
template <size_t N, size_t VerbLen>
constexpr const char* with_verb(const std::array<char, N>& base_path, const char (&verb)[VerbLen]) {
  static constexpr auto result = [&]() {  // Lambda to compute once
    std::array<char, N + VerbLen> arr{};
    size_t i = 0;
    for (size_t j = 0; j < N && base_path[j] != '\0'; ++j) arr[i++] = base_path[j];
    arr[i++] = '.';
    for (size_t j = 0; j < VerbLen - 1; ++j) arr[i++] = verb[j];
    arr[i] = '\0';
    return arr;
  }();
  return result.data();
}

}  // namespace topic

// Usage:
// PS_PUB_INT(topics::heater::chamber::target::set, 250);
// → "heater.chamber.target.set"

*/