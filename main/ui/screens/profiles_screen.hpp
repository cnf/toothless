#pragma once

#include <memory>
#include <string>
#include <vector>

#include "heater/profiles/profile.hpp"
#include "heater/profiles/profile_manager.hpp"
#include "screen.hpp"

extern "C" {
#include <pubsub.h>
}

namespace toothless {

struct ProfilesScreenLabels : public ScreenLabels {
  lv_obj_t* menu = nullptr;                    //<! Main Menu object
  lv_obj_t* root_page = nullptr;               //<! Root page of the menu
  lv_obj_t* keyboard = nullptr;                //<! On-screen keyboard
  lv_obj_t* profile_list_container = nullptr;  //<! Container for profile cards
  lv_obj_t* stages_list_container = nullptr;   //<! Container for stage cards
  lv_obj_t* backdrop;
  // lv_obj_t* profile_edit_page = nullptr;       //<! Page for editing profile

  // lv_obj_t* edit_name_ta = nullptr;
  // lv_obj_t* profile_stage_container = nullptr;
  // lv_obj_t* stage_edit_page = nullptr;
  // std::vector<lv_obj_t*> stage_cards;
};

struct ProfileEditContext {
  std::vector<std::string> profiles_list;  //<! list of profile names
  std::shared_ptr<Profile> profile;        //<! profile object being edited
  std::string profile_original_name;       //<! original name of profile being edited
  lv_obj_t* profile_page = nullptr;        //<! page for editing profile
  lv_obj_t* profile_name_ta = nullptr;
  // std::shared_ptr<Profile::Stage> stage;
  Profile::Stage* stage;
  std::vector<lv_obj_t*> stage_cards;
  size_t stage_index = 0;
  lv_obj_t* stage_page = nullptr;
  lv_obj_t* stage_name_ta = nullptr;
  lv_obj_t* spinbox;

  lv_obj_t* stage_from = nullptr;
  lv_obj_t* stage_to = nullptr;
  lv_obj_t* stage_duration = nullptr;
};

struct StageContext {};

class ProfilesScreen : public Screen {
 public:
  ProfilesScreen();
  ~ProfilesScreen();

  lv_obj_t* Create() override;
  void Loop() override;
  ScreenLabels* GetLabels() override { return _labels.get(); };

 private:
  std::unique_ptr<ProfilesScreenLabels> _labels;  //<! lvgl objects to act on for this screen
  std::unique_ptr<ProfileEditContext> _edit_ctx;  //<! context for editing profiles
  ps_subscriber_t* _subscription;
  std::shared_ptr<ProfileManager> _profile_mgr;

  // std::vector<std::string> _profile_names;
  std::string _selected_profile;  //<! profile selected to send to heater

  // std::shared_ptr<Profile> _editing_profile;
  // std::string _editing_profile_original_name;

  // std::shared_ptr<Profile::Stage> _editing_stage;
  // size_t _editing_stage_index = 0;

  // UI Creation
  esp_err_t CreateMenu();
  void RefreshProfileList();
  void RefreshStageList();
  lv_obj_t* CreateProfileCard(lv_obj_t* parent, const std::string& name);
  lv_obj_t* CreateProfileEditPage();
  void PopulateProfileEditPage();

  lv_obj_t* CreateStageCard(lv_obj_t* parent, const Profile::Stage& stage, size_t index);
  lv_obj_t* CreateStageEditPage();
  // void PopulateStageEditPage(Profile::Stage* stage);
  void PopulateStageEditPage(size_t index);

  // Event Handlers
  static void MenuBackHandler(lv_event_t* e);

  static void ProfileSelectHandler(lv_event_t* e);
  static void ProfileEditHandler(lv_event_t* e);
  static void ProfileDeleteHandler(lv_event_t* e);
  static void ProfileLoadHandler(lv_event_t* e);
  static void ProfileAddHandler(lv_event_t* e);
  static void ProfileSaveHandler(lv_event_t* e);

  static void StageAddHandler(lv_event_t* e);
  static void StageSaveHandler(lv_event_t* e);
  static void StageEditHandler(lv_event_t* e);
  static void StageEditUnitHandler(lv_event_t* e);
  static void StageDeleteHandler(lv_event_t* e);

  // static void PTextAreaEventHandler(lv_event_t* e);
};

}  // namespace toothless