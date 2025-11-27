#include "config_mgr.hpp"

#include <esp_app_desc.h>
#include <esp_task_wdt.h>
#include <esp_timer.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <nvs_flash.h>

#include <algorithm>
#include <numeric>
#include <tuple>  // for std::get

#include "topics.hpp"

#define LOG_LOCAL_LEVEL ESP_LOG_DEBUG
#include "funlog.h"

Preferences _prefs;

static uint32_t rt_counter = 0;

ConfigManager::ConfigManager() { snprintf(_register_topic, 30, "%s%s%s", kTopicConfig, TOPIC_DOT, kTopicRegister); }

/// @brief Create a TaskScheduler Task running the Loop()
/// @return shared pointer to the Task
// std::shared_ptr<TaskContext> ConfigManager::GetTask() {
//   this->Setup();
//   task_ = std::make_shared<TaskContext>(&ConfigManager::Loop, this);
//   return task_;
// }

void CfgMngrShim(void* pvParameters) {
  esp_task_wdt_add(NULL);
  while (true) {
    reinterpret_cast<ConfigManager*>(pvParameters)->Loop();
    esp_task_wdt_reset();
    vTaskDelay(pdMS_TO_TICKS(100));
  }
  FLOG_ERROR("Config Manager Loop exited, this should never happen!");
  PS_CALL_NIL(kTopicReset, 10000);  // reset the MCU if we crash/fail
}

void ConfigManager::Start() {
  FLOG_INFO("Starting Config Manager");
  this->Setup();
  xTaskCreatePinnedToCore(CfgMngrShim,         // Task function
                          "Config Manager",    // name of task
                          4096,                // Stack size of task, in bytes
                          this,                // parameter of the task
                          7,                   // priority of the task
                          &_core_task_handle,  // task handle
                          0);                  // MAIN_CPU
};

void ConfigManager::Setup() {
  // esp_log_level_set(FLOG_SHORT_FILENAME, ESP_LOG_DEBUG);
  // ESP_ERROR_CHECK(nvs_flash_erase());
  FLOG_DEBUG("Listening on %s", kTopicConfig);
  _subscriptions = ps_new_subscriber(10, PS_STRLIST(kTopicConfig));
  nvs_handle_t nvs_handle;
  esp_err_t err = nvs_open(kConfigManagerNamespace, NVS_READWRITE, &nvs_handle);
  nvs_close(nvs_handle);
  switch (err) {
    case ESP_ERR_NVS_NOT_INITIALIZED:
      FLOG_DEBUG("NVS not initialized, initializing now");
      // nvs_flash_init();
      InitializeNVS();
      break;
    case ESP_ERR_NVS_NOT_ENOUGH_SPACE:
      FLOG_ERROR("NVS not enough space, reformatting");
      InitializeNVS();
      break;
    case ESP_OK:
      FLOG_DEBUG("NVS opened successfully");
      break;
    default:
      FLOG_ERROR("NVS open failed with error:", esp_err_to_name(err));
  }

  if (!_prefs.begin(kConfigManagerNamespace)) {
    _prefs.end();
    FLOG_ERROR("WTF??? NVS wasn't initialized, doing it again...");
    InitializeNVS();
    return;
  };
  if (!_prefs.isKey("nvsInit")) {
    _prefs.end();
    FLOG_ERROR("nvsInit was not set");
    InitializeNVS();
    return;
  }
  _prefs.end();
  ReadNamespaces();
  std::string mod_list;
  RegisterSettings("system", &system_entries);  // TODO: make "system" a constant
  for (auto i : _nvs_namespaces) {
    if (mod_list.length() > 0) {
      mod_list += ", ";
    }
    mod_list += i;
  }
  FLOG_INFO("Found modules %s", mod_list.c_str());
}

void ConfigManager::Loop() {
  ps_msg_t* msg = NULL;
  TopicParts parts;
  for ((msg = ps_get(_subscriptions, 0)); msg != NULL; (msg = ps_get(_subscriptions, 0))) {
    ParseTopic(msg->topic, &parts);
    FLOG_DEBUG("Incoming Request: %s", msg->topic);
    // .format
    if (ps_has_topic(msg, kFormatTopic) && PS_IS_BOOL(msg) && msg->bool_val) {
      FLOG_WARN("Erasing NVS Flash on request");
      ESP_ERROR_CHECK(nvs_flash_erase());
      ps_unref_msg(msg);
      PS_PUB_BOOL(kTopicReset, true);  // This restarts the esp
      continue;                        // we are done here, the esp will restart and reinitialize NVS
      // .register
    } else if (ps_has_topic_suffix(msg, kTopicRegister)) {
      RegisterSettings(parts.module.c_str(), (ConfigEntries*)msg->ptr_val);
      if (msg->rtopic != NULL) {
        PS_PUB_NIL(msg->rtopic);
      }
      ps_unref_msg(msg);
      continue;
      // .get
    } else if (ps_has_topic_suffix(msg, "entries.get")) {
      auto it = _entries.find(parts.module.c_str());
      if (it != _entries.end()) {
        if (msg->rtopic != NULL) {
          PS_PUB_PTR(msg->rtopic, &(it->second));
        }
      } else {
        FLOG_WARN("No entries for namespace '%s'", parts.module.c_str());
        if (msg->rtopic != NULL) {
          PS_PUB_NIL(msg->rtopic);
        }
      }
      ps_unref_msg(msg);
      continue;
      // } else if (ps_has_topic_suffix(msg, ".entries")) {
      //   // Request for ConfigEntries metadata
      //   if (parts.module != "" && PS_IS_PTR(msg)) {
      //     // Caller provides a pointer to store entries
      //     ConfigEntries** out = (ConfigEntries**)msg->ptr_val;

      //     auto it = _entries.find(parts.module.c_str());
      //     if (it != _entries.end()) {
      //       *out = &it->second;  // Give pointer to our entries
      //     } else {
      //       *out = nullptr;
      //     }

      //     if (msg->rtopic != NULL) {
      //       PS_PUB_NIL(msg->rtopic);
      //     }
      //   }
    } else if (ps_has_topic_suffix(msg, TOPIC_DOT TOPIC_GET)) {
      if (parts.verb == kVerbGet && parts.module == "system") {
        FLOG_DEBUG("System Description requested");
        SystemDescription();
        ps_unref_msg(msg);
        continue;
      }
      FLOG_DEBUG("verb: %i, target: %s, module: %s, name: %s", (int)parts.verb, parts.target.c_str(),
                 parts.module.c_str(),
                 parts.name.c_str());  //, parts->verb);
      if (PS_IS_PTR(msg)) {
        FLOG_DEBUG("Loading settings for %s", parts.module.c_str());
        std::shared_ptr<SettingsMap> cfg_struct = *((std::shared_ptr<SettingsMap>*)msg->ptr_val);
        // std::shared_ptr<SettingsMap> cfg_struct = static_cast<std::shared_ptr<SettingsMap>>(msg->ptr_val);
        // uint8_t count = ReadSettings(parts.module.c_str(), cfg_struct);
        size_t count = ReadSettings(parts.module.c_str(), *((std::shared_ptr<SettingsMap>*)msg->ptr_val));
        if (count == 0) {
          // TODO: no settings loaded?
          FLOG_ERROR("No settings found for %s", parts.module.c_str());
        }

        char topic[kMaxTopicLength];
        parts.TopicSet(topic);
        snprintf(topic, kMaxTopicLength, "%s.%s", parts.module.c_str(), kTopicConfigSet);
        FLOG_DEBUG("Loaded %d settings for %s", count, topic);
        if (msg->rtopic != NULL) {
          PS_PUB_NIL(msg->rtopic);
        } else {
          PS_PUB_NIL(topic);
        }
      } else if (parts.module != "") {
        if (parts.name != "") {
          // FLOG_DEBUG("Showing config for %s", parts.name.c_str());
          SendPref(parts.module, parts.name);
        } else {
          // FLOG_DEBUG("Showing all config");
          SendPrefs(parts.module);
          // ShowPrefs(parts.module);
        }
      } else {
        SendPrefs();
        // FLOG_DEBUG("showing config");
        // ShowPrefs();
        // FLOG_DEBUG("Done showing config");
      }
      // .set
    } else if (ps_has_topic_suffix(msg, TOPIC_DOT TOPIC_SET)) {
      FLOG_DEBUG("Setting %s to new value", parts.name.c_str());
      switch (msg->flags & PS_MSK_TYP) {
        case PS_INT_TYP:
          WriteSetting(parts.module.c_str(), parts.name.c_str(), (int)msg->int_val);
          break;
        case PS_DBL_TYP:
          WriteSetting(parts.module.c_str(), parts.name.c_str(), (double)msg->dbl_val);
          break;
        case PS_BOOL_TYP:
          // FLOG_DEBUG("Setting %s to %i", parts.name.c_str(), (bool)msg->bool_val);
          WriteSetting(parts.module.c_str(), parts.name.c_str(), (bool)msg->bool_val);
          break;
        case PS_STR_TYP:
          WriteSetting(parts.module.c_str(), parts.name.c_str(), msg->str_val);
          break;
        default:
          FLOG_ERROR("Unknown type");
      }
      FLOG_DEBUG("Wrote %s", parts.name.c_str());
      char topic[kMaxTopicLength];
      parts.TopicGet(topic);
      FLOG_DEBUG("Informing %s about the config change", topic);
      PS_PUB_NIL(topic);
    } else if (ps_has_topic_suffix(msg, ".add")) {  // TODO: parametrize
      switch (msg->flags & PS_MSK_TYP) {
        case PS_STR_TYP:
          FLOG_DEBUG("Adding %s to %s.%s", msg->str_val, parts.module.c_str(), parts.name.c_str());
          AddToSetting(parts.module.c_str(), parts.name.c_str(), msg->str_val);
          // WriteSetting(parts.module.c_str(), parts.name.c_str(), msg->str_val);
          char topic[kMaxTopicLength];
          parts.TopicGet(topic);
          FLOG_DEBUG("Informing %s about the config change", topic);
          PS_PUB_NIL(topic);
          break;
        default:
          FLOG_ERROR("Unknown type, Can not Add");
      }
    } else if (ps_has_topic_suffix(msg, ".remove")) {  // TODO: parametrize
      switch (msg->flags & PS_MSK_TYP) {
        case PS_STR_TYP:
          FLOG_DEBUG("Adding %s to %s.%s", msg->str_val, parts.module.c_str(), parts.name.c_str());
          RemoveFromSetting(parts.module.c_str(), parts.name.c_str(), msg->str_val);
          char topic[kMaxTopicLength];
          parts.TopicGet(topic);
          FLOG_DEBUG("Informing %s about the config change", topic);
          PS_PUB_NIL(topic);
          break;
        default:
          FLOG_ERROR("Unknown type, Can not Add");
      }

    } else if (ps_has_topic_suffix(msg, ".reset")) {
      DeleteSetting(parts.module.c_str(), parts.name.c_str());
    } else {
      FLOG_DEBUG("How the hell did we get here????????");
    }
    // ps_unref_msg(msg);
    // portability::Wait(100); // wait 100ms so we give other tasks time to do their thing.
  }
  // FLOG_ERROR("Config Manager Loop exited, this should never happen!");
  // PS_CALL_NIL(kTopicReset, 10000); // reset the MCU if we crash/fail
  // BUG: we need a watchdog to make sure this doesn't crash/hang
}

size_t ConfigManager::RegisterSettings(const char* name_space, ConfigEntries* config_entries) {
  FLOG_DEBUG("Registering config entries for %s", name_space);
  size_t written = 0;
  std::string buf;
  AddNamespace(name_space);
  if (!_prefs.begin(name_space)) {
    _prefs.end();
    FLOG_DEBUG("Could not open namespace %s, Aborting...", name_space);
    return 0;
  };
  for (auto& element : *config_entries) {
    FLOG_DEBUG("Registering %s: type=%d, format='%s'", element.key, element.type, element.format.c_str());
    if (!element.format.empty()) {
      Validator v = ParseValidator(element.format);
      _validators[MakeValidatorKey(name_space, element.key)] = v;
    }
    if (!_prefs.isKey(element.key)) {  // TODO: atm, we don't check type, if it changes, manually format.
      FLOG_VERBOSE("New entry for %s of type %i", element.key, element.type);
      switch (element.type) {
        case ConfigValueTypes::kInt:
          written += _prefs.putInt(element.key, std::get<int>(element.default_value));
          break;
        case ConfigValueTypes::kDouble:
          written += _prefs.putDouble(element.key, std::get<double>(element.default_value));
          break;
        case ConfigValueTypes::kBool:
          written += _prefs.putBool(element.key, std::get<bool>(element.default_value));
          break;
        case ConfigValueTypes::kString:
          written += _prefs.putString(element.key, std::get<std::string>(element.default_value).c_str());
          break;
        // TODO: Figure out byte array
        case ConfigValueTypes::kBytes:
          FLOG_ERROR("Not implemented yet");
          //   _prefs.putBytes(element.key, std::get<std::vector<uint8_t>>(element.default_value),
          //                   strlen(std::get<std::vector<uint8_t>>(element.default_value)));
          break;
        case ConfigValueTypes::kVector:
          Serialize(std::get<std::vector<std::string>>(element.default_value), buf);
          FLOG_DEBUG("Register %s: %s", element.key, buf);
          written += _prefs.putString(element.key, buf);
          break;
        default:
          FLOG_ERROR("Unknown Type supplied.");
      }
    }

    _entries.insert_or_assign(name_space, *config_entries);
    // _entries[nspace] = *config_entries;
  };
  FLOG_DEBUG("Wrote %i bytes", written);
  _prefs.end();
  return written;
}

size_t ConfigManager::ReadSettings(const char* name_space, std::shared_ptr<SettingsMap> settings) {
  uint8_t count = 0;
  _prefs.begin(name_space, true);
  for (auto& entry : _entries[name_space]) {
    if (!_prefs.isKey(entry.key)) {
      FLOG_ERROR("Setting does not exist in NVS: %s", entry.key);
      continue;
    }
    count++;
    switch (entry.type) {
      case ConfigValueTypes::kInt:
        settings->emplace(entry.key, _prefs.getInt(entry.key, std::get<int>(entry.default_value)));
        break;
      case ConfigValueTypes::kDouble:
        settings->emplace(entry.key,
                          static_cast<double>(_prefs.getDouble(entry.key, std::get<double>(entry.default_value))));
        break;
      case ConfigValueTypes::kBool:
        settings->emplace(entry.key, _prefs.getBool(entry.key, std::get<bool>(entry.default_value)));
        break;
      case ConfigValueTypes::kString:
        settings->emplace(
            entry.key,
            std::string(_prefs.getString(entry.key, std::get<std::string>(entry.default_value).c_str()).c_str()));
        break;
      case ConfigValueTypes::kBytes:
        FLOG_ERROR("Not implemented yet");
        count--;
        break;
      case ConfigValueTypes::kVector:
        FLOG_DEBUG("Reading vector for %s in namespace %s", entry.key, name_space);
        {
          std::string serial_data = _prefs.getString(entry.key, "");
          ConfigVector vec;
          DeSerialize(&serial_data, vec);
          settings->emplace(entry.key, vec);
        }
        break;
      default:
        FLOG_ERROR("Unknown type");
        count--;
    }
  }
  _prefs.end();
  // printf("Loaded %i settings for %s", count, name_space);

  return count;
}

size_t ConfigManager::DeleteSetting(const char* name_space, const char* name) {
  _prefs.begin(name_space);
  _prefs.remove(name);
  _prefs.end();
  FLOG_INFO("Deleted `%s` in module `%s`", name, name_space);
  return 0;  // TODO: check if delete was successful
}

// template <typename T> size_t ConfigManager::WriteSetting(const char *name_space, const char *name, const T &value) {
//   _prefs.begin(name_space);
//   size_t size = _prefs.put<T>(name, value);
//   _prefs.end();
//   return size;
// }

size_t ConfigManager::WriteSetting(const char* name_space, const char* name, int value) {
  std::string vkey = MakeValidatorKey(name_space, name);
  if (_validators.count(vkey) && !_validators[vkey].validate_int(value)) {
    FLOG_ERROR("Validation failed for %s.%s: %d", name_space, name, value);
    return 0;
  }
  _prefs.begin(name_space);
  size_t size = _prefs.putInt(name, value);
  _prefs.end();
  return size;
  // return 0;
}

size_t ConfigManager::WriteSetting(const char* name_space, const char* name, double value) {
  std::string vkey = MakeValidatorKey(name_space, name);
  if (_validators.count(vkey) && !_validators[vkey].validate_double(value)) {
    FLOG_ERROR("Validation failed for %s.%s: %f", name_space, name, value);
    return 0;
  }
  _prefs.begin(name_space);
  size_t size = _prefs.putDouble(name, value);
  _prefs.end();
  return size;
}

size_t ConfigManager::WriteSetting(const char* name_space, const char* name, bool value) {
  std::string vkey = MakeValidatorKey(name_space, name);
  if (_validators.count(vkey) && !_validators[vkey].validate_bool(value)) {
    FLOG_ERROR("Validation failed for %s.%s: %s", name_space, name, value ? "true" : "false");
    return 0;
  }
  _prefs.begin(name_space);
  size_t size = _prefs.putBool(name, value);
  bool test = _prefs.getBool(name);
  if (test != value) {
    FLOG_DEBUG("Failed to set bool", name);
  }
  _prefs.end();
  return size;
}

size_t ConfigManager::WriteSetting(const char* name_space, const char* name, const char* value) {
  return WriteSetting(name_space, name, std::string(value));  // Delegate to std::string version
  // std::string vkey = MakeValidatorKey(name_space, name);
  // if (_validators.count(vkey) && !_validators[vkey].validate_string(value)) {
  //   FLOG_ERROR("Validation failed for %s.%s: %s", name_space, name, value);
  //   return 0;
  // }
  // _prefs.begin(name_space);
  // size_t size = _prefs.putString(name, value);
  // _prefs.end();
  // return size;
}

// size_t ConfigManager::WriteSetting(const char* name_space, const char* name, const std::string& value) {
//   std::string vkey = MakeValidatorKey(name_space, name);
//   if (_validators.count(vkey) && !_validators[vkey].validate_string(value)) {
//     FLOG_ERROR("Validation failed for %s.%s: %s", name_space, name, value.c_str());
//     return 0;
//   }
//   _prefs.begin(name_space);
//   size_t size = _prefs.putString(name, value);
//   _prefs.end();
//   return size;
// }

size_t ConfigManager::WriteSetting(const char* name_space, const char* name, const std::string& value) {
  size_t written = 0;

  if (!_prefs.begin(name_space)) {
    _prefs.end();
    FLOG_ERROR("Could not open namespace %s", name_space);
    return 0;
  }

  // **NORMALIZE STRING VALUE**
  std::string normalized = value;

  // Check if this key has a format constraint
  auto ns_it = _entries.find(name_space);
  if (ns_it != _entries.end()) {
    for (const auto& entry : ns_it->second) {
      if (std::string(entry.key) == std::string(name)) {
        if (entry.type == ConfigValueTypes::kString && !entry.format.empty()) {
          normalized = config_utils::ValidateAndNormalize(value, entry.format);
          if (normalized.empty()) {
            FLOG_ERROR("String '%s' invalid for %s.%s format: %s", value.c_str(), name_space, name,
                       entry.format.c_str());
            _prefs.end();
            return 0;
          }
        }
        break;
      }
    }
  }

  written = _prefs.putString(name, normalized.c_str());
  _prefs.end();

  // char topic[kMaxTopicLength];
  // snprintf(topic, kMaxTopicLength, "%s.%s.%s.%s", kTopicConfig, name_space, name, TOPIC_SET);
  // PS_PUB_STR(topic, normalized.c_str());

  return written;
}

size_t ConfigManager::WriteSetting(const char* name_space, const char* name, const ConfigVector& value) {
  // std::string vkey = MakeValidatorKey(name_space, name);
  // if (_validators.count(vkey) && !_validators[vkey].validate_string(value)) {
  //   FLOG_ERROR("Validation failed for %s.%s: %s", name_space, name, value.c_str());
  //   return 0;
  // }
  FLOG_DEBUG("Writing vector for %s in namespace %s", name, name_space);
  std::string serial_data;
  Serialize(value, serial_data);  // FIXME: check if serialization worked.
  _prefs.begin(name_space);
  size_t size = _prefs.putString(name, serial_data);
  _prefs.end();
  return size_t();
}

size_t ConfigManager::AddToSetting(const char* name_space, const char* name, const std::string& value) {
  const ConfigEntries entry = _entries[name_space];
  auto it = std::find_if(entry.begin(), entry.end(), [name](const ConfigEntry& e) { return strcmp(e.key, name) == 0; });
  if (it == entry.end()) {
    FLOG_DEBUG("Not Found %s.%s", name_space, name);
    return 0;
  }
  if (!std::holds_alternative<ConfigVector>(it->default_value)) {
    FLOG_DEBUG("Wrong Type: %s", ConfigValueTypeToString(it->type));
    return 0;
  }
  _prefs.begin(name_space);
  std::vector<std::string> vec;
  std::string current_value = _prefs.getString(name, "");
  DeSerialize(&current_value, vec);
  if (std::find(vec.begin(), vec.end(), value) == vec.end()) {  // only add if the value is not already in the list
    vec.push_back(value);
  } else {
    FLOG_ERROR("Value %s already exists in %s.%s, not adding again", value.c_str(), name_space, name);
    _prefs.end();
    return 0;  // nothing changed, so return 0
  }
  Serialize(vec, current_value);
  size_t size = _prefs.putString(name, current_value);
  _prefs.end();
  return size;
}

size_t ConfigManager::RemoveFromSetting(const char* name_space, const char* name, const std::string& value) {
  return size_t();
}

ConfigEntry ConfigManager::GetSetting(std::string name_space, std::string key) {
  if (!_entries.contains(name_space.c_str())) {
    FLOG_VERBOSE("Module %s not found", name_space.c_str());
    return ConfigEntry{};
  }
  if (_entries[name_space.c_str()].empty()) {
    FLOG_VERBOSE("No entries for module %s", name_space.c_str());
    return ConfigEntry{};
  }
  auto it = _entries.find(name_space.c_str());
  for (ConfigEntry& entry : it->second) {
    if (entry.key == key) return entry;
  }
  FLOG_VERBOSE("Key %s not found in %s", key.c_str(), name_space.c_str());
  return ConfigEntry{};
}

ConfigEntries ConfigManager::GetConfigEntries(const char* name_space) {
  auto it = _entries.find(name_space);
  if (it != _entries.end()) {
    return it->second;  // Return copy of entries
  }
  FLOG_WARN("No config entries found for namespace '%s'", name_space);
  return ConfigEntries();  // Return empty vector
}

void ConfigManager::SendPref(std::string module_name, std::string key) {
  // if (!_entries.contains(module_name.c_str())) {
  //   FLOG_ERROR("Module %s not found", module_name.c_str());
  //   return;
  // }
  // if (_entries[module_name.c_str()].empty()) {
  //   FLOG_ERROR("No entries for module %s", module_name.c_str());
  //   return;
  // }
  // ConfigEntry entry = GetSetting(module_name, key);
  // if (strlen(entry.key) == 0) {
  //   FLOG_ERROR("Key %s not found in module %s", key.c_str(), module_name.c_str());
  //   // delete msg;
  //   return;
  // }
  // FLOG_DEBUG("Getting config for %s.%s", module_name.c_str(), key.c_str());  // FIXME: remove
  // protofun::ConfigMessage* msg = new protofun::ConfigMessage();

  // _prefs.begin(module_name.c_str(), true);

  // msg->module_name = module_name;
  // msg->key = key;
  // msg->description = entry.description;
  // msg->format = entry.format;
  // msg->unit = entry.unit;

  // switch (entry.type) {
  //   case ConfigValueTypes::kInt:
  //     msg->value = std::to_string(_prefs.getInt(entry.key, std::get<int>(entry.default_value)));
  //     msg->default_value = std::to_string(std::get<int>(entry.default_value));
  //     break;
  //   case ConfigValueTypes::kDouble:
  //     msg->value = std::to_string(_prefs.getDouble(entry.key, std::get<double>(entry.default_value)));
  //     msg->default_value = std::to_string(std::get<double>(entry.default_value));
  //     break;
  //   case ConfigValueTypes::kBool:
  //     msg->value = _prefs.getBool(entry.key, std::get<bool>(entry.default_value)) ? "True" : "False";
  //     msg->default_value = std::get<bool>(entry.default_value) ? "True" : "False";
  //     break;
  //   case ConfigValueTypes::kString:
  //     msg->value = _prefs.getString(entry.key, std::get<std::string>(entry.default_value).c_str()).c_str();
  //     msg->default_value = std::get<std::string>(entry.default_value);
  //     break;
  //   case ConfigValueTypes::kVector: {
  //     std::string serialized;
  //     // ConfigVector vec;
  //     // msg->value = "Vectors / Lists Not Implemented yet";
  //     serialized = _prefs.getString(entry.key, "");
  //     FLOG_DEBUG("Serialized data: %s", serialized.c_str());
  //     // DeSerialize(&serialized, vec);
  //     msg->value = serialized;
  //     // msg->value = _prefs.getString(entry.key, std::get<std::string>(entry.default_value).c_str()).c_str();
  //     // msg->default_value = std::get<std::string>(entry.default_value);
  //     break;
  //   }
  //   default:
  //     msg->value = "Not Impl";
  // }
  // _prefs.end();
  // // PS_PUB_PTR(kTopicReport, msg); // FIXME: figure out the right topic
  // PS_PUB_BUF("report.config", msg, sizeof(protofun::ConfigMessage),
  //            [](void* p) { delete static_cast<protofun::ConfigMessage*>(p); });
  // FLOG_DEBUG("Sent config message for %s.%s: %s", module_name.c_str(), key.c_str(), msg->value.c_str());
}

void ConfigManager::SendPrefs(std::string module_name) {
  _prefs.end();
  for (auto& entry : _entries[module_name.c_str()]) {
    SendPref(module_name, entry.key);
  }
}

void ConfigManager::SendPrefs() {
  for (auto& el : _nvs_namespaces) {
    SendPrefs(el);
  }
}

void ConfigManager::ShowPrefs() {
  for (auto& el : _nvs_namespaces) {
    ShowPrefs(el);
  }
}

void ConfigManager::ShowPrefs(std::string name_space) {
  char cfg_line[180];
  if (!_prefs.begin(name_space.c_str(), true)) {
    FLOG_ERROR("No NVS space %s", name_space.c_str());
    _prefs.end();
    return;
  }
  std::string config_dump;
  // snprintf(cfg_line, 180, "%-15s %10s  %s", "Key", "Value", "Description");

  for (auto& entry : _entries[name_space.c_str()]) {
    std::string value;
    switch (entry.type) {
      case ConfigValueTypes::kInt:
        if (_prefs.isKey(entry.key)) {
          value = std::to_string(_prefs.getInt(entry.key, std::get<int>(entry.default_value)));
        } else {
          value = std::to_string(std::get<int>(entry.default_value));
        }
        break;
      case ConfigValueTypes::kDouble:
        if (_prefs.isKey(entry.key)) {
          value = std::to_string(_prefs.getInt(entry.key, std::get<double>(entry.default_value)));
        } else {
          value = std::to_string(std::get<double>(entry.default_value));
        }
        break;
      case ConfigValueTypes::kBool:
        if (_prefs.getBool(entry.key, std::get<bool>(entry.default_value))) {
          value = "True";
        } else {
          value = "False";
        }
        break;
      case ConfigValueTypes::kString:
        value = _prefs.getString(entry.key, std::get<std::string>(entry.default_value).c_str()).c_str();
        break;
      default:
        value = "Not Impl";
    }
    snprintf(cfg_line, 180, "%-15s %10s  %-s", entry.key, value.c_str(), entry.description.c_str());
    // PRINTLN(cfg_line);
    // PS_PUB_STR(kTopicReport, cfg_line);
  }
  // _prefs.end();
}

uint32_t ConfigManager::ParseTopic(const char* topic, TopicParts* parts) {
  char topic_copy[kMaxTopicLength];
  strncpy(topic_copy, topic, kMaxTopicLength);
  const char* del = TOPIC_DOT;
  uint8_t counter = 0;
  bool verbed = false;

  char* t = strtok(topic_copy, del);
  if (t == nullptr) {
    return counter;
  }

  if (strcmp(t, TOPIC_INC_PREFIX) == 0) {
    parts->incoming = true;
    t = strtok(nullptr, del);
  }

  while (t != nullptr) {
    if (strcmp(t, kTopicVerbSet) == 0) {
      if (!verbed) {
        parts->verb = TopicVerbs::kVerbSet;
        verbed = true;
      }
      return counter;
    } else if (strcmp(t, kTopicVerbGet) == 0) {
      if (!verbed) {
        parts->verb = TopicVerbs::kVerbGet;
        verbed = true;
      }
    } else if (strcmp(t, kTopicVerbErase) == 0) {
      if (!verbed) {
        parts->verb = TopicVerbs::kVerbErase;
        verbed = true;
      }
    } else {
      switch (counter) {
        case 0:
          parts->target = t;
          counter++;
          break;
        case 1:
          parts->module = t;
          counter++;
          break;
        case 2:
          parts->name = t;
          counter++;
          break;
        default:
          break;
      }
    }
    t = strtok(nullptr, del);
  }
  return counter;
}

bool ConfigManager::InitializeNVS() {
  FLOG_INFO("Initializing Non-Volatile Storage");
  _prefs.end();
  // FLOG_DEBUG("Erasing NVS Flash");
  // ESP_ERROR_CHECK(nvs_flash_erase()); // TODO: do we need to erase?
  FLOG_DEBUG("Initializing NVS Flash");
  ESP_ERROR_CHECK(nvs_flash_init());
  FLOG_DEBUG("Opening NVS Namespace %s", kConfigManagerNamespace);
  _prefs.begin(kConfigManagerNamespace);
  _prefs.putBool("nvsInit", true);
  _prefs.end();
  // FLOG_DEBUG("Adding NVS Namespace", kConfigManagerNamespace);
  // AddNamespace(kConfigManagerNamespace);
  // FLOG_DEBUG("RESETTING ESP32");
  // PS_PUB_BOOL(kTopicReset, true);
  return false;
}

bool ConfigManager::AddNamespace(std::string name_space) {
  if (std::find(_nvs_namespaces.begin(), _nvs_namespaces.end(), name_space) != _nvs_namespaces.end()) {
    return true;
  }
  _nvs_namespaces.push_back(name_space);
  std::sort(_nvs_namespaces.begin(), _nvs_namespaces.end());
  return WriteNamespaces();
}

bool ConfigManager::ReadNamespaces() {
  FLOG_DEBUG("Reading NVS Namespaces");
  _prefs.end();
  if (!_prefs.begin(kConfigManagerNamespace, true)) {
    FLOG_DEBUG("Can not open namespace %s", kConfigManagerNamespace);
    _prefs.end();
    return false;
  }
  char dump[512] = {};

  if (!_prefs.isKey("namespaces")) {
    FLOG_DEBUG("No namespaces found");
    _prefs.end();
    WriteNamespaces();
    return false;
  }

  size_t arr_size = _prefs.getBytesLength("namespaces");
  _prefs.getBytes("namespaces", dump, arr_size);
  const char* del = ".";
  char* t = strtok(dump, del);
  while (t != nullptr) {
    _nvs_namespaces.push_back(t);
    t = strtok(nullptr, del);
  }
  _prefs.end();
  return false;
}

bool ConfigManager::WriteNamespaces() {
  if (!_prefs.begin(kConfigManagerNamespace)) {
    FLOG_DEBUG("Can not open namespace %s", kConfigManagerNamespace);
    _prefs.end();
    return false;
  }

  std::string buff =
      std::accumulate(_nvs_namespaces.begin(), _nvs_namespaces.end(), std::string(),
                      [](const std::string& ss, const std::string& s) { return ss.empty() ? s : ss + "." + s; });

  FLOG_DEBUG("Writing namespaces %s", buff.c_str());
  _prefs.putBytes("namespaces", buff.c_str(), buff.length() + 1);
  _prefs.end();
  return true;
}
/////////////////////////////
// Add to config_mgr.cpp:

Validator ConfigManager::ParseValidator(const std::string& format) {
  Validator v;
  if (format.empty()) return v;

  // Simple parser for "key=val,key=val" format
  size_t pos = 0;
  while (pos < format.size()) {
    size_t eq = format.find('=', pos);
    if (eq == std::string::npos) break;

    std::string key = format.substr(pos, eq - pos);
    size_t comma = format.find(',', eq);
    if (comma == std::string::npos) comma = format.size();
    std::string val = format.substr(eq + 1, comma - eq - 1);

    if (key == "min") {
      v.min_val = std::stod(val);
      v.has_min = true;
    } else if (key == "max") {
      v.max_val = std::stod(val);
      v.has_max = true;
    } else if (key == "enum") {
      v.has_enum = true;
      size_t start = 0;
      while (start < val.size()) {
        size_t pipe = val.find('|', start);
        if (pipe == std::string::npos) pipe = val.size();
        v.enum_vals.push_back(val.substr(start, pipe - start));
        start = pipe + 1;
      }
    } else if (key == "len") {
      size_t colon = val.find(':');
      if (colon != std::string::npos) {
        v.min_len = std::stoul(val.substr(0, colon));
        v.max_len = std::stoul(val.substr(colon + 1));
        v.has_len = true;
      }
    }

    pos = comma + 1;
  }
  return v;
}

std::string ConfigManager::MakeValidatorKey(const char* ns, const char* key) { return std::string(ns) + "." + key; }

void ConfigManager::SystemDescription() {
  const esp_app_desc_t* system = esp_app_get_description();
  char desc[200] = {0};
  snprintf(desc, 199, "%s :: %s, compiled %s %s [%s]", system->project_name, system->version, system->date,
           system->time, esp_app_get_elf_sha256_str());
  PS_PUB_STR(kTopicReport, desc);
}

void RegisterConfig(ConfigEntries* config_entries, const char* name) {
  char t_topic[kMaxTopicLength];
  snprintf(t_topic, kMaxTopicLength, "%s.%s.%s", kTopicConfig, name, kTopicRegister);
  PS_CALL_PTR(t_topic, config_entries, 10000);
}

// const ConfigEntries* GetConfigEntries(const char* name_space) {
//   // char topic[kMaxTopicLength];
//   // char rtopic[32];
//   // ConfigEntries* result = nullptr;

//   // snprintf(topic, kMaxTopicLength, "config.%s.entries", name_space);
//   // snprintf(rtopic, sizeof(rtopic), "$r.config.%u", ++rt_counter);

//   // ps_subscriber_t* su = ps_new_subscriber(1, PS_STRLIST(rtopic));

//   // ps_msg_t* req = ps_new_msg(topic, PS_PTR_TYP, &result);
//   // ps_msg_set_rtopic(req, rtopic);
//   // ps_publish(req);

//   // // Wait for response
//   // time_t timer = portability::Millis() + 5000;
//   // while (portability::Millis() < timer) {
//   //   ps_msg_t* resp = ps_get(su, 0);
//   //   if (resp != NULL) {
//   //     ps_unref_msg(resp);
//   //     break;
//   //   }
//   //   portability::Wait(10);
//   // }

//   // ps_free_subscriber(su);
//   // return result;
// }

void GetSettings(std::shared_ptr<SettingsMap>& config, const char* name) {
  FLOG_DEBUG("Getting settings for %s", name);
  if (!config) {
    // config = std::make_shared<SettingsMap>();
    FLOG_ERROR("Config pointer is null for %s", name);
    return;
  }
  config->clear();
  char t_topic[kMaxTopicLength];
  char r_topic[32] = {0};
  ps_msg_t* ret_msg = NULL;

  snprintf(t_topic, kMaxTopicLength, "%s.%s.%s", kTopicConfig, name, TOPIC_GET);
  // snprintf(r_topic, sizeof(r_topic), "$r.config.%lu", ++rt_counter);
  // ps_subscriber_t* su = ps_new_subscriber(1, PS_STRLIST(r_topic));

  // ps_msg_t* config_msg = ps_new_msg(t_topic, PS_PTR_TYP, (void*)(&config));
  // ps_msg_set_rtopic(config_msg, r_topic);
  // ps_publish(config_msg);
  ret_msg = PS_CALL_PTR(t_topic, (void*)(&config), 3000);
  // time_t timer = esp_timer_get_time() + 3 * 1000 * 1000;  // wait for 3 seconds
  // while (esp_timer_get_time() < timer) {
  //   ret_msg = ps_get(su, 0);
  //   if (ret_msg != NULL) {
  //     break;
  //     // if (PS_IS_NIL(msg)) {
  //     //   FLOG_INFO("Config received back for %s", name);
  //     //   ps_unref_msg(msg);
  //     //   ps_free_subscriber(su);
  //     //   return;
  //     // } else if (PS_IS_PTR(msg)) {
  //     //   FLOG_DEBUG("Config received for %s", name);
  //     //   *config = *((std::shared_ptr<SettingsMap> *)msg->ptr_val);
  //     //   ps_unref_msg(msg);
  //     //   ps_free_subscriber(su);
  //     //   return;
  //     // } else {
  //     //   FLOG_ERROR("Wrong type received for %s", name);
  //     // }
  //   }
  //   vTaskDelay(pdMS_TO_TICKS(10));
  // }

  // ps_msg_t *msg = PS_CALL_PTR(t_topic, &config, 10000);
  if (ret_msg != NULL && PS_IS_NIL(ret_msg)) {
    FLOG_DEBUG("%s config received back", name);
    ps_unref_msg(ret_msg);
    return;
  } else {
    FLOG_ERROR("Config FAILED for %s", name);
  }
  ps_unref_msg(ret_msg);
}

ConfigEntries GetConfigEntries(const char* name_space) {
  char t_topic[kMaxTopicLength];
  ConfigEntries entries;

  snprintf(t_topic, kMaxTopicLength, "%s.%s.%s", kTopicConfig, name_space, "entries.get");
  ps_msg_t* msg = PS_CALL_PTR(t_topic, nullptr, 10000);

  if (msg == NULL) {
    FLOG_ERROR("Failed to get config entries for '%s'", name_space);
    return entries;
  }

  if (PS_IS_PTR(msg)) {
    entries = *((ConfigEntries*)msg->ptr_val);
  }

  ps_unref_msg(msg);
  return entries;
}

size_t Serialize(const std::vector<std::string>& vector, std::string& serialized) {
  serialized =
      std::accumulate(vector.begin(), vector.end(), std::string(),
                      [](const std::string& ss, const std::string& s) { return ss.empty() ? s : ss + "." + s; });
  return serialized.length();
}

// size_t DeSerialize(const std::string* serialized, std::vector<std::string>& vector) {
//   vector.clear();
//   const char* del = ".";
//   char* t = strtok((char*)serialized->c_str(), del);
//   while (t != nullptr) {
//     vector.push_back(t);
//     t = strtok(nullptr, del);
//   }
//   return vector.size();
// }

size_t DeSerialize(const std::string* serialized, std::vector<std::string>& vector) {
  vector.clear();
  if (serialized->empty()) return 0;

  size_t start = 0;
  size_t end = serialized->find('.');

  while (end != std::string::npos) {
    vector.push_back(serialized->substr(start, end - start));
    start = end + 1;
    end = serialized->find('.', start);
  }
  vector.push_back(serialized->substr(start));  // Last element

  return vector.size();
}