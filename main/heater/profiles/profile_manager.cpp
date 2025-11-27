#include "profile_manager.hpp"

#include <dirent.h>
#include <sys/stat.h>

#include <algorithm>
#include <cstring>

#include "esp_littlefs.h"
#include "funlog.h"

extern "C" {
#include <pubsub.h>
}

namespace toothless {

constexpr const char* kStorageBasePath = "/storage";
constexpr const char* kProfilesPath = "/storage/profiles";
constexpr const char* kProfileExtension = ".json";

ProfileManager::ProfileManager() : _initialized(false) { Init(); }

ProfileManager::~ProfileManager() {
  if (_initialized) {
    esp_vfs_littlefs_unregister("storage");
  }
}

std::shared_ptr<ProfileManager> ProfileManager::GetInstance() {
  static std::shared_ptr<ProfileManager> instance(new ProfileManager());
  return instance;
}

esp_err_t ProfileManager::Init() {
  if (_initialized) {
    FLOG_WARN("ProfileManager already initialized");
    return ESP_OK;
  }

  // Mount LittleFS
  esp_vfs_littlefs_conf_t conf = {
      .base_path = kStorageBasePath,
      .partition_label = "storage",
      .format_if_mount_failed = true,
      .dont_mount = false,
  };

  esp_err_t ret = esp_vfs_littlefs_register(&conf);
  if (ret != ESP_OK) {
    FLOG_ERROR("Failed to mount LittleFS: %s", esp_err_to_name(ret));
    return ret;
  }

  size_t total = 0, used = 0;
  ret = esp_littlefs_info("storage", &total, &used);
  if (ret == ESP_OK) {
    FLOG_INFO("LittleFS: %zu/%zu bytes used", used, total);
  }

  // Create profiles directory if it doesn't exist
  struct stat st;
  if (stat(kProfilesPath, &st) != 0) {
    if (mkdir(kProfilesPath, 0755) != 0) {
      FLOG_ERROR("Failed to create profiles directory");
      return ESP_FAIL;
    }
    FLOG_INFO("Created profiles directory");
  }

  _initialized = true;

  // Load existing profiles from filesystem
  ret = LoadAllProfiles();
  if (ret != ESP_OK) {
    FLOG_WARN("Failed to load profiles: %s", esp_err_to_name(ret));
  }

  // Create defaults if no profiles exist
  if (_profiles.empty()) {
    FLOG_INFO("No profiles found, creating defaults");
    CreateDefaults();
  }

  return ESP_OK;
}

std::shared_ptr<Profile> ProfileManager::GetProfile(const std::string& name) {
  // **NORMALIZE LOOKUP KEY**
  std::string normalized = name;
  std::replace(normalized.begin(), normalized.end(), ' ', '_');
  std::transform(normalized.begin(), normalized.end(), normalized.begin(), ::tolower);

  auto it = _profiles.find(normalized);
  if (it != _profiles.end()) {
    return it->second;
  }
  FLOG_WARN("Profile '%s' (normalized: '%s') not found", name.c_str(), normalized.c_str());
  return nullptr;
}

std::vector<std::string> ProfileManager::ListProfiles() {
  std::vector<std::string> names;
  names.reserve(_profiles.size());
  for (const auto& [name, profile] : _profiles) {
    names.push_back(name);
  }
  std::sort(names.begin(), names.end());
  return names;
}

esp_err_t ProfileManager::SaveProfile(const std::string& name, std::shared_ptr<Profile> profile) {
  if (!_initialized) {
    FLOG_ERROR("ProfileManager not initialized");
    return ESP_ERR_INVALID_STATE;
  }

  if (name.empty()) {
    FLOG_ERROR("Profile name cannot be empty");
    return ESP_ERR_INVALID_ARG;
  }

  // **NORMALIZE NAME FOR STORAGE**
  std::string normalized = name;
  std::replace(normalized.begin(), normalized.end(), ' ', '_');
  std::transform(normalized.begin(), normalized.end(), normalized.begin(), ::tolower);

  profile->SetName(name);

  // Save to filesystem
  esp_err_t ret = SaveToFile(normalized, profile);
  if (ret != ESP_OK) {
    return ret;
  }

  // Update in-memory cache with NORMALIZED key
  _profiles[normalized] = profile;
  FLOG_INFO("Saved profile '%s' (normalized: '%s')", name.c_str(), normalized.c_str());

  // Notify that profiles changed
  PS_PUB_NIL("profiles.changed");

  return ESP_OK;
}

esp_err_t ProfileManager::DeleteProfile(const std::string& name) {
  if (!_initialized) {
    FLOG_ERROR("ProfileManager not initialized");
    return ESP_ERR_INVALID_STATE;
  }

  std::string filepath = std::string(kProfilesPath) + "/" + name + kProfileExtension;

  if (unlink(filepath.c_str()) != 0) {
    FLOG_ERROR("Failed to delete profile file '%s'", filepath.c_str());
    return ESP_FAIL;
  }

  _profiles.erase(name);
  FLOG_INFO("Deleted profile '%s'", name.c_str());

  // **NOTIFY THAT PROFILES CHANGED**
  PS_PUB_NIL("profiles.changed");

  return ESP_OK;
}

esp_err_t ProfileManager::CreateDefaults() {
  // Qwik Lead Free profile
  std::vector<Profile::Stage> lead_free_stages = {
      {25, 150, 90000, Profile::Shape::Smooth, "Preheat"},
      {150, 180, 90000, Profile::Shape::Linear, "Soak"},
      {180, 240, 30000, Profile::Shape::Smooth, "Ramp to Peak"},
      {240, 100, 120000, Profile::Shape::Smooth, "Cooldown"},
  };
  auto lead_free = std::make_shared<Profile>("Chip Quik Lead Free", lead_free_stages);
  SaveProfile("chip_quik_lead_free", lead_free);

  // Qwik Leaded profile
  std::vector<Profile::Stage> leaded_stages = {
      {25, 90, 90000, Profile::Shape::Smooth, "Preheat"},    {90, 130, 90000, Profile::Shape::Linear, "Soak"},
      {130, 138, 45000, Profile::Shape::Smooth, "Ramp Up"},  {138, 165, 30000, Profile::Shape::Smooth, "Reflow"},
      {165, 100, 30000, Profile::Shape::Linear, "Cooldown"},
  };
  auto leaded = std::make_shared<Profile>("Chip Quik Leaded", leaded_stages);
  SaveProfile("chip_quik_leaded", leaded);

  FLOG_INFO("Created default profiles");
  return ESP_OK;
}

esp_err_t ProfileManager::LoadAllProfiles() {
  DIR* dir = opendir(kProfilesPath);
  if (!dir) {
    FLOG_WARN("Cannot open profiles directory");
    return ESP_FAIL;
  }

  struct dirent* entry;
  int loaded = 0;

  while ((entry = readdir(dir)) != nullptr) {
    if (entry->d_type != DT_REG) continue;

    std::string filename(entry->d_name);
    if (filename.find(kProfileExtension) == std::string::npos) continue;

    // Remove extension to get profile name
    std::string name = filename.substr(0, filename.find_last_of('.'));

    if (LoadFromFile(name) == ESP_OK) {
      loaded++;
    }
  }

  closedir(dir);
  FLOG_INFO("Loaded %d profile(s)", loaded);

  return ESP_OK;
}

esp_err_t ProfileManager::LoadFromFile(const std::string& name) {
  std::string filepath = std::string(kProfilesPath) + "/" + name + kProfileExtension;

  FILE* f = fopen(filepath.c_str(), "r");
  if (!f) {
    FLOG_ERROR("Cannot open profile file '%s'", filepath.c_str());
    return ESP_FAIL;
  }

  // Get file size
  fseek(f, 0, SEEK_END);
  long fsize = ftell(f);
  fseek(f, 0, SEEK_SET);

  if (fsize <= 0 || fsize > 16384) {  // Sanity check: max 16KB
    FLOG_ERROR("Invalid file size: %ld", fsize);
    fclose(f);
    return ESP_FAIL;
  }

  // Read entire file
  std::string json_str;
  json_str.resize(fsize);
  size_t read = fread(&json_str[0], 1, fsize, f);
  fclose(f);

  if (read != static_cast<size_t>(fsize)) {
    FLOG_ERROR("Failed to read profile file");
    return ESP_FAIL;
  }

  // Parse JSON and create profile
  auto profile = Profile::FromJSON(json_str);
  if (!profile) {
    FLOG_ERROR("Failed to parse profile JSON");
    return ESP_FAIL;
  }

  _profiles[name] = profile;
  FLOG_DEBUG("Loaded profile '%s' from %s", name.c_str(), filepath.c_str());

  return ESP_OK;
}

esp_err_t ProfileManager::SaveToFile(const std::string& name, std::shared_ptr<Profile> profile) {
  if (!profile) {
    FLOG_ERROR("Cannot save null profile");
    return ESP_ERR_INVALID_ARG;
  }

  std::string filepath = std::string(kProfilesPath) + "/" + name + kProfileExtension;
  std::string json_str = profile->ToJSON();

  FILE* f = fopen(filepath.c_str(), "w");
  if (!f) {
    FLOG_ERROR("Cannot create profile file '%s'", filepath.c_str());
    return ESP_FAIL;
  }

  size_t written = fwrite(json_str.c_str(), 1, json_str.length(), f);
  fclose(f);

  if (written != json_str.length()) {
    FLOG_ERROR("Failed to write profile file");
    return ESP_FAIL;
  }

  FLOG_DEBUG("Saved profile '%s' to %s (%zu bytes)", name.c_str(), filepath.c_str(), written);

  return ESP_OK;
}

}  // namespace toothless