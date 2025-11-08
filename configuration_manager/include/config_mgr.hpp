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
 * @author
 * @date
 */
// cSpell: words _config_mngr
#pragma once

#include "config.h"

#include "preferences.hpp"
// #include <QDispatch.h>
// #include "topics.h"
#include <cstdint>
#include <cstring>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <map>
#include <memory>
#include <variant>
#include <vector>

extern "C" {
#include <pubsub.h>
}

static constexpr char kConfigManagerNamespace[] = "_config_mngr";
static constexpr uint8_t kMaxNvsNameLength = 15;

// char re[32]

enum ConfigValueTypes { kInt, kDouble, kBool, kString, kBytes, kVector };
inline const char *ConfigValueTypeToString(ConfigValueTypes type) {
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

// ConfigVector::Print() {
//   std::string result;
//   for (const auto &item : *this) {
//     if (!result.empty()) {
//       result += ", ";
//     }
//     result += item;
//   }
//   return result;
// }

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

  ConfigEntry(const char nkey[kMaxNvsNameLength], std::string descr, std::string format, int default_val,
              std::string unit) {
    strncpy(key, nkey, kMaxNvsNameLength);
    description = descr;
    format = format;
    type = ConfigValueTypes::kInt;
    default_value = default_val;
    unit = unit;
  }
  ConfigEntry(const char nkey[kMaxNvsNameLength], std::string descr, std::string format, double default_val,
              std::string iunit) {
    strncpy(key, nkey, kMaxNvsNameLength);
    description = descr;
    format = format;
    type = ConfigValueTypes::kDouble;
    default_value = default_val;
    unit = iunit;
  }
  ConfigEntry(const char nkey[kMaxNvsNameLength], std::string descr, std::string format, bool default_val,
              std::string iunit) {
    strncpy(key, nkey, kMaxNvsNameLength);
    description = descr;
    format = format;
    type = ConfigValueTypes::kBool;
    default_value = default_val;
    unit = iunit;
  }
  ConfigEntry(char nkey[kMaxNvsNameLength], std::string description, std::string format, std::string default_val,
              std::string iunit) {
    strncpy(key, nkey, kMaxNvsNameLength);
    description = description;
    format = format;
    type = ConfigValueTypes::kString;
    default_value = default_val;
    unit = iunit;
  }
  ConfigEntry(const char nkey[kMaxNvsNameLength], std::string descr, std::string format,
              const ConfigVector &default_val, std::string iunit) {
    strncpy(key, nkey, kMaxNvsNameLength);
    description = descr;
    format = format;
    type = ConfigValueTypes::kVector;
    default_value = default_val;
    unit = iunit;
  }
  ConfigEntry() {
    key[0] = '\0'; // Initialize key to an empty string
    description = "";
    format = "";
    type = ConfigValueTypes::kString; // Default type
    default_value = "";               // Default value as an empty string
    unit = "";
  }
  // TODO: Bytes not implemented yet in config manager
};

using ConfigEntries = std::vector<ConfigEntry>;

using ConfigMap = std::map<std::string, ConfigEntries>;
using SettingsMap = std::map<std::string, ConfigValue>;

enum TopicVerbs { kVerbGet, kVerbSet, kVerbErase };
using TopicParts = struct {
  bool incoming;
  std::string target; // probably "config" as it is us.
  std::string module; // NVS namespace
  std::string name;   // config key
  TopicVerbs verb;
  // size_t Topic(char *buf) {
  //   char topic[kMaxTopicLength];
  //   snprintf(topic, kMaxTopicLength, "%s.%s", module.c_str(), kTopicConfig);
  //   return strlen(topic);
  // };
  size_t TopicGet(char *buf) {
    // char topic[kMaxTopicLength];
    snprintf(buf, kMaxTopicLength, "%s.%s", module.c_str(), kTopicConfigGet);
    return strlen(buf);
  };

  size_t TopicSet(char *buf) {
    // char topic[kMaxTopicLength];
    snprintf(buf, kMaxTopicLength, "%s.%s", module.c_str(), kTopicConfigSet);
    return strlen(buf);
  };
};

struct SystemConfig {
  bool test;
  ConfigVector test_vector; // for testing purposes, this is not used in the system.
};

inline ConfigEntries system_entries = {
    ConfigEntry("test", "Test entry", "true/false", false, ""),
    ConfigEntry("test_vector", "Testing Vectors", "aliasa.aliasb", ConfigVector(), "")

};

class ConfigManager {
private:
  // std::map<char[15], std::vector<ConfigEntry>> _entries;
  ConfigMap _entries;
  std::vector<std::string> nvs_namespaces_;
  ps_subscriber_t *config_sub_;
  char register_topic_[kMaxTopicLength];
  // std::string register_topic_;
  // const std::string GetNamespaceFromTopic(const char *topic);
  TaskHandle_t _core_task_handle;
  uint32_t ParseTopic(const char *topic, TopicParts *parts);
  // TODO: MUTEX
  bool InitializeNVS();
  bool AddNamespace(std::string name_space);
  bool ReadNamespaces();
  bool WriteNamespaces();

  static constexpr const char kFormatTopic[] = "inc.config.format"; // FIXME: make config options

public:
  ConfigManager();
  // ~ConfigManager();
  // std::shared_ptr<TaskContext> GetTask();
  void Setup();
  void Start();
  void Loop();
  size_t RegisterSettings(const char *name_space, ConfigEntries *config_entries);
  size_t ReadSettings(const char *name_space, std::shared_ptr<SettingsMap> settings);
  size_t DeleteSetting(const char *name_space, const char *name);
  // template <typename T> size_t WriteSetting(const char *name_space, const char *name, const T &value);
  size_t WriteSetting(const char *name_space, const char *name, int value);
  size_t WriteSetting(const char *name_space, const char *name, double value);
  size_t WriteSetting(const char *name_space, const char *name, bool value);
  size_t WriteSetting(const char *name_space, const char *name, const char *value);
  size_t WriteSetting(const char *name_space, const char *name, const std::string &value);
  size_t WriteSetting(const char *name_space, const char *name, const std::vector<std::string> &value);
  size_t AddToSetting(const char *name_space, const char *name, const std::string &value);
  size_t RemoveFromSetting(const char *name_space, const char *name, const std::string &value);
  ConfigEntry GetSetting(std::string name_space, std::string key);
  void ShowPrefs(std::string name_space);
  void ShowPrefs();
  void SendPref(std::string module_name, std::string key);
  void SendPrefs(std::string module_name);
  void SendPrefs();

  void SystemDescription();
};

void CfgMngrShim(void *pvParameters);

void RegisterConfig(ConfigEntries *config_entries, const char *name);
void GetSettings(std::shared_ptr<SettingsMap> &_config, const char *name);
size_t Serialize(const std::vector<std::string> &vector, std::string &serialized);
size_t DeSerialize(const std::string *serialized, std::vector<std::string> &vector);