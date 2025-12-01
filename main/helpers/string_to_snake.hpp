#pragma once

#include <cctype>
#include <string>

inline std::string StringToSnake(const std::string& string_case) {
  std::string result;

  for (size_t i = 0; i < string_case.length(); i++) {
    char c = string_case[i];

    if (c == ' ') {
      result += '_';
    } else if (std::isupper(c)) {
      // Add underscore before uppercase if not first char and prev wasn't underscore
      if (i > 0 && result.back() != '_') {
        result += '_';
      }
      result += std::tolower(c);
    } else {
      result += c;
    }
  }

  return result;
}