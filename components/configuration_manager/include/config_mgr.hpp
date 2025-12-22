/**
 * @file config_mgr.hpp
 * @brief Configuration Manager for handling system and module settings.
 *
 * This header defines the ConfigManager class and related types for managing configuration
 * entries, namespaces, and settings in an embedded system. It supports multiple value types,
 * persistent storage, and topic-based communication for configuration operations.
 *
 * Key Components:
 * - ConfigValueTypes: Enum for supported configuration value types (int, double, bool, string, bytes, vector).
 * - ConfigEntry: Structure representing a single configuration entry, including key, description, type, default value,
 * and unit.
 * - ConfigEntries: Vector of ConfigEntry objects.
 * - ConfigMap: Map from namespace (string) to ConfigEntries.
 * - SettingsMap: Map from key (string) to ConfigValue.
 * - TopicParts: Structure for parsing and constructing topic strings for configuration messages.
 * - SystemConfig: Example structure for system-level configuration.
 * - ConfigManager: Main class for registering, reading, writing, and managing configuration settings.
 *
 * Features:
 * - Register and manage configuration namespaces and entries.
 * - Read, write, and delete settings with type safety.
 * - Serialize and deserialize vector settings.
 * - Topic-based communication for configuration operations (get, set, erase).
 * - Integration with FreeRTOS tasks and pubsub messaging.
 *
 * Usage:
 * - Define configuration entries for each module or namespace.
 * - Register entries with the ConfigManager.
 * - Use provided methods to manipulate and query settings at runtime.
 *
 * @author Frank Rosquin
 * @date 2025
 */
// cSpell: words _config_mngr
#pragma once
// clang-format off
#include "topics.hpp"  //<! include these first
// clang-format on

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include <cstdint>
#include <cstring>
#include <map>
#include <memory>
#include <variant>
#include <vector>

#include "cfgmgr_topics.hpp"
#include "funlog.h"
#include "preferences.hpp"

extern "C" {
#include <pubsub.h>
}

// Add these utility functions after the includes, before the namespace
namespace config_utils {

/// @brief Convert string to lower_snake_case
inline std::string NormalizeString(const std::string& str) {
  std::string result;
  result.reserve(str.length());

  bool prev_lower = false;
  bool first_char = true;

  for (char c : str) {
    if (c == ' ' || c == '-') {
      result += '_';
      prev_lower = false;
      first_char = false;
    } else if (std::isupper(c)) {
      // Add underscore before uppercase if previous was lowercase
      if (!first_char && prev_lower) {
        result += '_';
      }
      result += std::tolower(c);
      prev_lower = false;
      first_char = false;
    } else {
      result += std::tolower(c);
      prev_lower = true;
      first_char = false;
    }
  }

  return result;
}

/// @brief Validate and normalize string value based on format
/// @return Normalized string, or empty if validation fails
inline std::string ValidateAndNormalize(const std::string& value, const std::string& format) {
  // Check if it's an enum format
  if (format.find("enum=") != std::string::npos) {
    std::string normalized_value = NormalizeString(value);

    // Extract valid values
    size_t pos = format.find("enum=") + 5;
    size_t end = format.find(',', pos);
    if (end == std::string::npos) end = format.size();

    std::string enum_part = format.substr(pos, end - pos);

    // Check if normalized value matches any normalized enum option
    size_t start = 0;
    while (start < enum_part.size()) {
      size_t pipe = enum_part.find('|', start);
      if (pipe == std::string::npos) pipe = enum_part.size();

      std::string option = enum_part.substr(start, pipe - start);
      std::string normalized_option = NormalizeString(option);

      if (normalized_option == normalized_value) {
        return normalized_value;
      }
      start = pipe + 1;
    }
    return "";
  }

  // **NOT AN ENUM - RETURN AS-IS, NO NORMALIZATION**
  return value;
}
// inline std::string ValidateAndNormalize(const std::string& value, const std::string& format) {
//   // Check if it's an enum format
//   if (format.find("enum=") != std::string::npos) {
//     std::string normalized = NormalizeString(value);

//     // Extract valid values
//     size_t pos = format.find("enum=") + 5;
//     size_t end = format.find(',', pos);
//     if (end == std::string::npos) end = format.size();

//     std::string enum_part = format.substr(pos, end - pos);

//     // Check if normalized value is in the enum
//     size_t start = 0;
//     while (start < enum_part.size()) {
//       size_t pipe = enum_part.find('|', start);
//       if (pipe == std::string::npos) pipe = enum_part.size();

//       std::string option = enum_part.substr(start, pipe - start);
//       if (NormalizeString(option) == normalized) {
//         return normalized;  // Valid enum value - return normalized
//       }
//       start = pipe + 1;
//     }

//     // Not found in enum - validation failed
//     return "";
//   }

//   // **NOT AN ENUM - RETURN AS-IS, NO NORMALIZATION**
//   return value;
// }

}  // namespace config_utils

static constexpr const char* kTopicIncPrefix = TOPIC_INC_PREFIX;
static constexpr const char* kTopicDelimiter = TOPIC_DOT;
static constexpr const char* kTopicVerbSet = TOPIC_VERB_SET;
static constexpr const char* kTopicVerbGet = TOPIC_VERB_GET;
static constexpr const char* kTopicVerbErase = TOPIC_VERB_ERASE;

namespace topics::config {
static constexpr const char name[15] = "config";
static constexpr const char get[19] = "config.get";
static constexpr const char set[19] = "config.set";
static constexpr const char erase[22] = "config.erase";
}  // namespace topics::config

static constexpr char kConfigManagerNamespace[] = "_config_mngr";
static constexpr uint8_t kMaxNvsNameLength = 15;

enum ConfigValueTypes { kInt, kDouble, kBool, kString, kBytes, kVector };
inline const char* ConfigValueTypeToString(ConfigValueTypes type) {
  switch (type) {
    case ConfigValueTypes::kInt:
      return "kInt";
    case ConfigValueTypes::kDouble:
      return "kDouble";
    case ConfigValueTypes::kBool:
      return "kBool";
    case ConfigValueTypes::kString:
      return "kString";
    case ConfigValueTypes::kBytes:
      return "kBytes";
    case ConfigValueTypes::kVector:
      return "kVector";
    default:
      return "Unknown";
  }
}

using ConfigVector = std::vector<std::string>;

using ConfigValue = std::variant<int, double, bool, std::string, std::vector<uint8_t>, ConfigVector>;

/// @brief  Configuration entry structure.
/// This structure defines a configuration entry with a unique key, description, format, type, default value, and unit.
/// @struct ConfigEntry
/// @param key Unique key to identify the entry (max length defined by kMaxNvsNameLength).
/// @param description Description of the configuration entry.
/// @param format Format string that describes the expected value format.
/// @param type Type of the value (int, double, bool, string, bytes, vector).
/// @param default_value Default value for the configuration entry, stored as a ConfigValue variant.
/// @param unit Unit of the value, e.g., "RPM", "m/s", "A".
struct ConfigEntry {
  char key[15];              /*!< unique key to identify the entry */
  std::string description;   /*!< description */
  std::string format;        /*!< the format this msg is limited to*/
  ConfigValueTypes type;     /*!< type of the value */
  ConfigValue default_value; /*!< */
  std::string unit = "";     /*!< unit of the value, e.g. "RPM", "m/s", "A" */

  /// @brief Constructor for ConfigEntry with int default value.
  /// @param nkey
  /// @param descr
  /// @param fmt
  /// @param default_val
  /// @param iunit
  ConfigEntry(const char nkey[kMaxNvsNameLength], std::string descr, std::string fmt, int default_val,
              std::string iunit) {
    strncpy(key, nkey, kMaxNvsNameLength);
    description = descr;
    format = fmt;
    type = ConfigValueTypes::kInt;
    default_value = default_val;
    unit = iunit;
  }
  ConfigEntry(const char nkey[kMaxNvsNameLength], std::string descr, std::string fmt, double default_val,
              std::string iunit) {
    strncpy(key, nkey, kMaxNvsNameLength);
    description = descr;
    format = fmt;
    type = ConfigValueTypes::kDouble;
    default_value = default_val;
    unit = iunit;
  }
  ConfigEntry(const char nkey[kMaxNvsNameLength], std::string descr, std::string fmt, bool default_val,
              std::string iunit) {
    strncpy(key, nkey, kMaxNvsNameLength);
    description = descr;
    format = fmt;
    type = ConfigValueTypes::kBool;
    default_value = default_val;
    unit = iunit;
  }
  ConfigEntry(const char nkey[kMaxNvsNameLength], std::string descr, std::string fmt, std::string default_val,
              std::string iunit) {
    strncpy(key, nkey, kMaxNvsNameLength);
    description = descr;
    format = fmt;
    type = ConfigValueTypes::kString;
    default_value = default_val;
    unit = iunit;
  }
  ConfigEntry(const char nkey[kMaxNvsNameLength], std::string descr, std::string fmt, const ConfigVector& default_val,
              std::string iunit) {
    strncpy(key, nkey, kMaxNvsNameLength);
    description = descr;
    format = fmt;
    type = ConfigValueTypes::kVector;
    default_value = default_val;
    unit = iunit;
  }
  ConfigEntry() {
    key[0] = '\0';  // Initialize key to an empty string
    description = "";
    format = "";
    type = ConfigValueTypes::kString;  // Default type
    default_value = "";                // Default value as an empty string
    unit = "";
  }
  // TODO: Bytes not implemented yet in config manager
};

/// @brief Validator for configuration values parsed from format string.
/// Format grammar:
///   min=0,max=100        - numeric range (int/double)
///   enum=off|on|auto     - allowed string values
///   len=1:32             - string length range
///   Multiple rules: "min=0,max=100,len=1:10"
struct Validator {
  bool has_min = false, has_max = false, has_enum = false, has_len = false;
  double min_val = 0.0, max_val = 0.0;
  size_t min_len = 0, max_len = 0;
  std::vector<std::string> enum_vals;
  bool password = false;  // for string password field

  bool validate_int(int value) const {
    if (has_min && value < min_val) return false;
    if (has_max && value > max_val) return false;
    return true;
  }

  bool validate_double(double value) const {
    if (has_min && value < min_val) return false;
    if (has_max && value > max_val) return false;
    return true;
  }

  bool validate_string(const std::string& value) const {
    if (has_len && (value.size() < min_len || value.size() > max_len)) return false;
    if (has_enum && std::find(enum_vals.begin(), enum_vals.end(), value) == enum_vals.end()) return false;
    return true;
  }

  bool validate_bool(bool value) const {
    // Bools always valid unless enum restricts to specific string repr
    return true;
  }
};

using ConfigEntries = std::vector<ConfigEntry>;

using ConfigMap = std::map<std::string, ConfigEntries>;
using SettingsMap = std::map<std::string, ConfigValue>;

enum TopicVerbs { kVerbGet, kVerbSet, kVerbErase };
using TopicParts = struct {
  bool incoming;
  std::string target;  // probably "config" as it is us.
  std::string module;  // NVS namespace
  std::string name;    // config key
  TopicVerbs verb;
  // size_t Topic(char *buf) {
  //   char topic[kMaxTopicLength];
  //   snprintf(topic, kMaxTopicLength, "%s.%s", module.c_str(), kTopicConfig);
  //   return strlen(topic);
  // };
  size_t TopicGet(char* buf) {
    // char topic[kMaxTopicLength];
    snprintf(buf, kMaxTopicLength, "%s.%s", module.c_str(), kTopicConfigGet);
    return strlen(buf);
  };

  size_t TopicSet(char* buf) {
    // char topic[kMaxTopicLength];
    snprintf(buf, kMaxTopicLength, "%s.%s", module.c_str(), kTopicConfigSet);
    return strlen(buf);
  };
};

struct SystemConfig {
  bool test;
  ConfigVector test_vector;  // for testing purposes, this is not used in the system.
};

inline ConfigEntries system_entries = {
    ConfigEntry("test", "Test entry", "true/false", false, ""),
    ConfigEntry("test_vector", "Testing Vectors", "aliasa.aliasb", ConfigVector(), "")

};

class ConfigManager {
 private:
  static inline std::shared_ptr<ConfigManager> _instance = nullptr;
  static TaskHandle_t _core_task_handle;
  // static inline std::atomic_bool _ready{false};
  // std::map<char[15], std::vector<ConfigEntry>> _entries;
  ConfigMap _entries;
  std::vector<std::string> _nvs_namespaces;
  ps_subscriber_t* _subscriptions;
  char _register_topic[kMaxTopicLength];
  std::map<std::string, Validator> _validators;  // key: "namespace.key"
  // std::string _register_topic;
  // const std::string GetNamespaceFromTopic(const char *topic);
  uint32_t ParseTopic(const char* topic, TopicParts* parts);
  // TODO: MUTEX
  bool InitializeNVS();
  bool AddNamespace(std::string name_space);
  bool ReadNamespaces();
  bool WriteNamespaces();
  std::string MakeValidatorKey(const char* ns, const char* key);

  static constexpr const char kFormatTopic[] = "config.format";  // FIXME: make config options

 public:
  ConfigManager();
  // ~ConfigManager();
  // std::shared_ptr<TaskContext> GetTask();
  static void Start();
  static void StarterTask(void*);

  void Setup();
  void Loop();
  size_t RegisterSettings(const char* name_space, ConfigEntries* config_entries);
  size_t ReadSettings(const char* name_space, std::shared_ptr<SettingsMap> settings);
  size_t DeleteSetting(const char* name_space, const char* name);
  // template <typename T> size_t WriteSetting(const char *name_space, const char *name, const T &value);
  size_t WriteSetting(const char* name_space, const char* name, int value);
  size_t WriteSetting(const char* name_space, const char* name, double value);
  size_t WriteSetting(const char* name_space, const char* name, bool value);
  size_t WriteSetting(const char* name_space, const char* name, const char* value);
  size_t WriteSetting(const char* name_space, const char* name, const std::string& value);
  size_t WriteSetting(const char* name_space, const char* name, const std::vector<std::string>& value);
  size_t AddToSetting(const char* name_space, const char* name, const std::string& value);
  size_t RemoveFromSetting(const char* name_space, const char* name, const std::string& value);
  ConfigEntry GetSetting(std::string name_space, std::string key);
  ConfigEntries GetConfigEntries(const char* name_space);
  void ShowPrefs(std::string name_space);
  void ShowPrefs();
  void SendPref(std::string module_name, std::string key);
  void SendPrefs(std::string module_name);
  void SendPrefs();
  void SystemDescription();

  static Validator ParseValidator(const std::string& format);
};

// void CfgMngrShim(void* pvParameters);

void RegisterConfig(ConfigEntries* config_entries, const char* name);
// const ConfigEntries* GetConfigEntries(const char* name_space);
void GetSettings(std::shared_ptr<SettingsMap>& _config, const char* name);
ConfigEntries GetConfigEntries(const char* name_space);
size_t Serialize(const std::vector<std::string>& vector, std::string& serialized);
size_t DeSerialize(const std::string* serialized, std::vector<std::string>& vector);