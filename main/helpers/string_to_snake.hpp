#pragma once

#include <cctype>
#include <string>

// // FIXME: this isn't quite right for all cases, but works for most simple ones. should handle TLAs better.
// inline std::string StringToSnake(const std::string& string_case) {
//   std::string result;

//   for (size_t i = 0; i < string_case.length(); i++) {
//     char c = string_case[i];

//     if (c == ' ') {
//       result += '_';
//     } else if (std::isupper(c)) {
//       // Add underscore before uppercase if not first char and prev wasn't underscore
//       if (i > 0 && result.back() != '_') {
//         result += '_';
//       }
//       result += std::tolower(c);
//     } else {
//       result += c;
//     }
//   }

//   return result;
// }

inline std::string StringToSnake(const std::string& string_case) {
  std::string result;

  for (size_t i = 0; i < string_case.length(); i++) {
    char c = string_case[i];

    if (c == ' ') {
      result += '_';
    } else if (std::isupper(c)) {
      // Check if we need underscore before this uppercase
      bool need_underscore = false;

      if (i > 0 && result.back() != '_') {
        char prev = string_case[i - 1];

        // Add underscore if previous was lowercase or digit
        if (std::islower(prev) || std::isdigit(prev)) {
          need_underscore = true;
        }
        // Add underscore if this is last char of acronym (next is lowercase)
        else if (std::isupper(prev) && i + 1 < string_case.length() && std::islower(string_case[i + 1])) {
          need_underscore = true;
        }
      }

      if (need_underscore) result += '_';
      result += std::tolower(c);
    } else if (std::isdigit(c)) {
      // Add underscore before digit if previous wasn't digit or underscore
      if (i > 0 && result.back() != '_' && !std::isdigit(string_case[i - 1])) {
        result += '_';
      }
      result += c;
    } else {
      result += c;
    }
  }

  return result;
}