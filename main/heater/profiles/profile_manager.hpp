#pragma once

#include <esp_err.h>

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "heater/profiles/profile.hpp"

namespace toothless {

namespace topics::profile {
const char name[8] = "profile";
const char changed[20] = "profile.changed";
}  // namespace topics::profile

/// @brief Manages reflow profiles with LittleFS persistence
class ProfileManager {
 public:
  ProfileManager();
  ~ProfileManager();

  /// @brief Get singleton instance
  /// @return Shared pointer to ProfileManager instance
  static std::shared_ptr<ProfileManager> GetInstance();

  /// @brief Initialize LittleFS and load existing profiles
  /// @return ESP_OK on success
  esp_err_t Init();

  /// @brief Get profile by name
  /// @param name Profile name
  /// @return Profile pointer or nullptr if not found
  std::shared_ptr<Profile> GetProfile(const std::string& name);

  /// @brief List all available profile names
  /// @return Vector of profile names, sorted alphabetically
  std::vector<std::string> ListProfiles();

  /// @brief Save profile to filesystem and cache
  /// @param name Profile name (used as filename)
  /// @param profile Profile to save
  /// @return ESP_OK on success
  esp_err_t SaveProfile(const std::string& name, std::shared_ptr<Profile> profile);

  /// @brief Delete profile from filesystem and cache
  /// @param name Profile name to delete
  /// @return ESP_OK on success
  esp_err_t DeleteProfile(const std::string& name);

  /// @brief Create default reflow profiles
  /// @return ESP_OK on success
  esp_err_t CreateDefaults();

 private:
  std::map<std::string, std::shared_ptr<Profile>> _profiles;
  bool _initialized;

  /// @brief Load all profiles from filesystem
  esp_err_t LoadAllProfiles();

  /// @brief Load single profile from JSON file
  /// @param name Profile name (without .json extension)
  esp_err_t LoadFromFile(const std::string& name);

  /// @brief Save profile to JSON file
  /// @param name Profile name
  /// @param profile Profile to save
  esp_err_t SaveToFile(const std::string& name, std::shared_ptr<Profile> profile);
};

}  // namespace toothless