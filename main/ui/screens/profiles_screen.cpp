#include "profiles_screen.hpp"

#include <algorithm>
#include <cmath>
#include <memory>

#include "funlog.h"
#include "heater/heater.hpp"
#include "ui/screens/screen_helpers.hpp"
#include "ui/themes/widget_factories.hpp"

extern "C" {
#include <pubsub.h>
}

namespace toothless {

ProfilesScreen::ProfilesScreen()
    : _labels(std::make_unique<ProfilesScreenLabels>()),
      _edit_ctx(std::make_unique<ProfileEditContext>()),
      _subscription(nullptr) {}

ProfilesScreen::~ProfilesScreen() {
  if (_subscription) {
    ps_free_subscriber(_subscription);
  }
}

lv_obj_t* ProfilesScreen::Create() {
  FLOG_INFO("Creating Profiles Screen");

  _screen = ui::CreateScreen();
  lv_obj_set_style_pad_all(_screen, 0, 0);

  _profile_mgr = ProfileManager::GetInstance();
  _profile_mgr->Init();
  CreateMenu();

  // Subscribe to profile changes
  _subscription = ps_new_subscriber(10, PS_STRLIST("profiles.changed"));

  return _screen;
}

void ProfilesScreen::Loop() {
  // Handle profile changes
  ps_msg_t* msg = NULL;
  for ((msg = ps_get(_subscription, 0)); msg != NULL; (msg = ps_get(_subscription, 0))) {
    if (ps_has_topic(msg, "profiles.changed")) {
      FLOG_INFO("Profiles changed, refreshing list");
      RefreshProfileList();
    }
    ps_unref_msg(msg);
  }
}

esp_err_t ProfilesScreen::CreateMenu() {
  _labels->menu = ui::CreateMenu(_screen, "Profiles");

  // Back button
  lv_menu_set_mode_root_back_button(_labels->menu, LV_MENU_ROOT_BACK_BUTTON_ENABLED);
  lv_obj_add_event_cb(_labels->menu, MenuBackHandler, LV_EVENT_CLICKED, this);
  lv_obj_t* back_btn = lv_menu_get_main_header_back_button(_labels->menu);
  lv_obj_set_ext_click_area(back_btn, lv_pct(33));

  // Root page
  _labels->root_page = ui::CreateMenuRootPage(_labels->menu, "Reflow Profiles");
  lv_menu_set_sidebar_page(_labels->menu, NULL);  // No sidebar
  lv_obj_set_size(_labels->root_page, lv_pct(100), lv_pct(100));
  lv_obj_remove_flag(_labels->root_page, LV_OBJ_FLAG_SCROLLABLE);

  // Main scrollable section
  lv_obj_t* sectionp = ui::CreateMenuRootSection(_labels->root_page);
  lv_obj_set_width(sectionp, lv_pct(100));
  // lv_obj_set_height(sectionp, lv_pct(100));
  lv_obj_set_flex_grow(sectionp, 1);  // Fill remaining space
  lv_obj_add_flag(sectionp, LV_OBJ_FLAG_SCROLLABLE);

  // Disable overscroll/bounce
  lv_obj_set_scroll_dir(sectionp, LV_DIR_VER);               // Only vertical scroll
  lv_obj_remove_flag(sectionp, LV_OBJ_FLAG_SCROLL_ELASTIC);  // No bounce/elastic
  // lv_obj_clear_flag(sectionp, LV_OBJ_FLAG_SCROLL_MOMENTUM);  // No momentum scroll (optional)

  // Profile list container
  _labels->profile_list_container = ui::CreateColumnContainer(sectionp);
  lv_obj_set_width(_labels->profile_list_container, lv_pct(100));
  lv_obj_set_height(_labels->profile_list_container, LV_SIZE_CONTENT);

  lv_obj_t* button_section = ui::CreateMenuRootSection(_labels->root_page);
  lv_obj_set_width(button_section, lv_pct(100));
  lv_obj_set_height(button_section, LV_SIZE_CONTENT);
  lv_obj_t* add_btn =
      ui::CreatePrimaryButton(button_section, LV_SYMBOL_PLUS " New Profile", lv_pct(100), LV_DPX(60), true);
  lv_obj_add_event_cb(add_btn, ProfileAddHandler, LV_EVENT_CLICKED, this);
  RefreshProfileList();

  lv_menu_set_page(_labels->menu, _labels->root_page);

  return ESP_OK;
}

void ProfilesScreen::RefreshProfileList() {
  // Clear existing profile cards
  lv_obj_clean(_labels->profile_list_container);

  // Get updated profile list
  _edit_ctx->profiles_list = _profile_mgr->ListProfiles();

  if (_edit_ctx->profiles_list.empty()) {
    lv_obj_t* empty_label = ui::CreateBodyText(_labels->profile_list_container, "No profiles found");
    lv_obj_set_style_text_align(empty_label, LV_TEXT_ALIGN_CENTER, 0);
    return;
  }

  // Create card for each profile
  for (const auto& name : _edit_ctx->profiles_list) {
    CreateProfileCard(_labels->profile_list_container, name);
    // lv_menu_separator_create(_labels->profile_list_container);
  }
}

void ProfilesScreen::RefreshStageList() {
  // Clear existing UI children (lv_obj_clean deletes all children)
  if (_labels->stages_list_container) {
    lv_obj_clean(_labels->stages_list_container);
  }
  // We no longer own the lv_obj pointers in _edit_ctx->stage_cards,
  // so clear the vector without deleting each card (avoids double free).
  _edit_ctx->stage_cards.clear();

  // Invalidate any in-flight stage pointer to avoid UAF later.
  _edit_ctx->stage = nullptr;
  _edit_ctx->stage_index = 0;

  // Get updated stage count
  size_t stage_count = _edit_ctx->profile->StageCount();
  if (stage_count == 0) {
    lv_obj_t* empty_label = ui::CreateBodyText(_labels->stages_list_container, "No stages found");
    lv_obj_set_style_text_align(empty_label, LV_TEXT_ALIGN_CENTER, 0);
    return;
  }

  // Create card for each stage
  for (size_t i = 0; i < _edit_ctx->profile->StageCount(); ++i) {
    const Profile::Stage* stage = _edit_ctx->profile->GetStage(i);
    if (stage) {
      CreateStageCard(_labels->stages_list_container, *stage, i);
    }
  }
}

lv_obj_t* ProfilesScreen::CreateProfileCard(lv_obj_t* parent, const std::string& name) {
  lv_obj_t* card = ui::CreateCard(parent);
  lv_obj_set_width(card, lv_pct(100));
  lv_obj_set_height(card, LV_SIZE_CONTENT);
  lv_obj_set_style_pad_all(card, 12, 0);

  // LEFT side: name and details
  lv_obj_t* info_wrapper = ui::CreateColumnContainer(card);
  lv_obj_set_flex_grow(info_wrapper, 1);  // Takes remaining space
  lv_obj_set_height(info_wrapper, LV_SIZE_CONTENT);

  lv_obj_set_style_pad_gap(info_wrapper, 5, 0);
  ui::CreateHeading(info_wrapper, ui::SnakeToTitle(name).c_str());

  // Profile info
  auto profile = _profile_mgr->GetProfile(name);
  if (profile) {
    // lv_obj_set_style_pad_gap(info_wrapper, 5, 0);
    std::string info = std::to_string(profile->StageCount()) + " stages, " +
                       std::to_string(profile->TotalDuration() / 1000) + "s total";
    ui::CreateSmallText(info_wrapper, info.c_str());
  }

  // Buttons row
  lv_obj_t* btn_row = ui::CreateRowContainer(card);
  lv_obj_set_width(btn_row, LV_SIZE_CONTENT);
  lv_obj_set_height(btn_row, LV_SIZE_CONTENT);
  lv_obj_set_style_pad_gap(btn_row, 8, 0);
  lv_obj_set_style_flex_main_place(btn_row, LV_FLEX_ALIGN_START, 0);

  // Edit button
  lv_obj_t* edit_btn = ui::CreateSecondaryButton(btn_row, LV_SYMBOL_EDIT, LV_SIZE_CONTENT, LV_SIZE_CONTENT, false);
  lv_obj_set_user_data(edit_btn, (void*)name.c_str());
  lv_obj_add_event_cb(edit_btn, ProfileEditHandler, LV_EVENT_CLICKED, this);

  // Duplicate button
  lv_obj_t* dup_btn = ui::CreateSecondaryButton(btn_row, LV_SYMBOL_DOWNLOAD, LV_SIZE_CONTENT, LV_SIZE_CONTENT, false);
  lv_obj_set_user_data(dup_btn, (void*)name.c_str());
  lv_obj_add_event_cb(dup_btn, ProfileLoadHandler, LV_EVENT_CLICKED, this);

  // Delete button
  lv_obj_t* del_btn = ui::CreateDangerButton(btn_row, LV_SYMBOL_TRASH, LV_SIZE_CONTENT, LV_SIZE_CONTENT, false);
  lv_obj_set_user_data(del_btn, (void*)name.c_str());
  lv_obj_add_event_cb(del_btn, ProfileDeleteHandler, LV_EVENT_CLICKED, this);

  // lv_menu_set_load_page_event(_labels->menu, cont, sub_page);// FIXME: sub pages

  return card;
}

lv_obj_t* ProfilesScreen::CreateProfileEditPage() {
  if (_edit_ctx->profile_page) {
    lv_obj_delete(_edit_ctx->profile_page);
    _edit_ctx->profile_page = nullptr;
  }
  _edit_ctx->profile_page = ui::CreateMenuPage(_labels->menu, "Edit Profile");
  lv_obj_t* section = ui::CreateMenuSection(_edit_ctx->profile_page);

  // Profile name input
  lv_obj_t* name_card = ui::CreateCard(section);
  lv_obj_set_width(name_card, lv_pct(100));
  lv_obj_set_height(name_card, LV_SIZE_CONTENT);

  ui::CreateBodyText(name_card, "Name:");
  _edit_ctx->profile_name_ta = ui::CreateTextArea(name_card);  // lv_textarea_create(name_card);
  lv_obj_add_event_cb(_edit_ctx->profile_name_ta, ProfileSaveHandler, LV_EVENT_READY, this);
  // lv_obj_set_width(_edit_ctx->profile_name_ta, lv_pct(100));
  lv_obj_set_flex_grow(_edit_ctx->profile_name_ta, 1);
  lv_textarea_set_one_line(_edit_ctx->profile_name_ta, true);
  lv_textarea_set_max_length(_edit_ctx->profile_name_ta, 32);

  // Add keyboard event handler
  lv_obj_add_event_cb(_edit_ctx->profile_name_ta, TextAreaEventHandler, LV_EVENT_FOCUSED, this);
  lv_obj_add_event_cb(_edit_ctx->profile_name_ta, TextAreaEventHandler, LV_EVENT_DEFOCUSED, this);
  lv_obj_add_event_cb(_edit_ctx->profile_name_ta, TextAreaEventHandler, LV_EVENT_READY, this);

  // Save button
  // lv_obj_t* save_btn = ui::CreateSuccessButton(wrapper, LV_SYMBOL_SAVE " Save Profile", lv_pct(50), 60, true);
  // lv_obj_add_event_cb(save_btn, ProfileSaveHandler, LV_EVENT_CLICKED, this);

  section = ui::CreateMenuSection(_edit_ctx->profile_page);
  lv_obj_set_flex_grow(section, 1);  // Fill remaining space
  lv_obj_add_flag(section, LV_OBJ_FLAG_SCROLLABLE);

  // Disable overscroll/bounce
  lv_obj_set_scroll_dir(section, LV_DIR_VER);               // Only vertical scroll
  lv_obj_remove_flag(section, LV_OBJ_FLAG_SCROLL_ELASTIC);  // No bounce/elastic

  // Stages section
  // ui::CreateHeading(section, "Stages");  // Changed from profile_edit_page to section

  // Container for stage cards (will be populated later)
  _labels->stages_list_container = ui::CreateColumnContainer(section);
  lv_obj_set_width(_labels->stages_list_container, lv_pct(100));
  lv_obj_set_height(_labels->stages_list_container, LV_SIZE_CONTENT);

  lv_obj_t* wrapper = ui::CreateRowContainer(section);
  lv_obj_set_width(wrapper, lv_pct(100));
  lv_obj_set_height(wrapper, LV_SIZE_CONTENT);

  // Add stage button
  lv_obj_t* add_stage_btn = ui::CreatePrimaryButton(wrapper, LV_SYMBOL_PLUS " Add Stage", lv_pct(50), 60, true);
  lv_obj_add_event_cb(add_stage_btn, StageAddHandler, LV_EVENT_CLICKED, this);

  lv_menu_set_page(_labels->menu, _edit_ctx->profile_page);
  // lv_menu_set_load_page_event(_labels->menu, cont, sub_page);

  return _edit_ctx->profile_page;
}

void ProfilesScreen::PopulateProfileEditPage() {
  if (!_edit_ctx->profile) return;

  // Set name
  lv_textarea_set_text(_edit_ctx->profile_name_ta, _edit_ctx->profile->Name().c_str());

  RefreshStageList();

  // // Clear existing stage cards
  // lv_obj_clean(_labels->stages_list_container);
  // _edit_ctx->stage_cards.clear();

  // // Create stage cards in the container
  // for (size_t i = 0; i < _edit_ctx->profile->StageCount(); ++i) {
  //   const Profile::Stage* stage = _edit_ctx->profile->GetStage(i);
  //   if (stage) {
  //     CreateStageCard(_labels->stages_list_container, *stage, i);
  //   }
  // }
}

lv_obj_t* ProfilesScreen::CreateStageCard(lv_obj_t* parent, const Profile::Stage& stage, size_t index) {
  lv_obj_t* card = ui::CreateCard(parent);
  lv_obj_set_width(card, lv_pct(100));
  lv_obj_set_height(card, LV_SIZE_CONTENT);
  lv_obj_set_style_pad_all(card, 12, 0);
  // lv_obj_set_flex_flow(card, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_style_flex_cross_place(card, LV_FLEX_ALIGN_CENTER, 0);

  // // Stage header
  // lv_obj_t* header_row = ui::CreateRowContainer(card);
  // lv_obj_set_width(header_row, lv_pct(100));
  // lv_obj_set_height(header_row, LV_SIZE_CONTENT);
  // // lv_obj_set_style_border_width(header_row, 3, 0);
  // // lv_obj_set_style_border_color(header_row, lv_color_hex(0x009900), 0);

  lv_obj_t* details = ui::CreateColumnContainer(card);
  // lv_obj_set_width(details, lv_pct(100));
  lv_obj_set_flex_grow(details, 1);

  std::string title = "Stage " + std::to_string(index + 1) + ": " + stage.name;
  lv_obj_t* obj = ui::CreateBodyText(details, title.c_str());
  lv_obj_set_height(obj, LV_SIZE_CONTENT);
  // lv_obj_set_flex_grow(obj, 1);

  std::string temp_info = std::to_string((int)stage.start_temp) + "°C " + LV_SYMBOL_RIGHT + " " +
                          std::to_string((int)stage.end_temp) + "°C" + " / " +
                          std::to_string(stage.duration_ms / 1000) + "s, " +
                          (stage.shape == Profile::Shape::Smooth ? "Smooth" : "Linear");
  ui::CreateSmallText(details, temp_info.c_str());

  // Edit button
  lv_obj_t* edit_btn = ui::CreateSecondaryButton(card, LV_SYMBOL_EDIT, LV_SIZE_CONTENT, LV_SIZE_CONTENT, false);
  lv_obj_set_user_data(edit_btn, (void*)index);
  lv_obj_add_event_cb(edit_btn, StageEditHandler, LV_EVENT_CLICKED, this);
  // Delete stage button
  lv_obj_t* del_btn = ui::CreateDangerButton(card, LV_SYMBOL_TRASH, false);
  lv_obj_set_user_data(del_btn, (void*)index);
  lv_obj_add_event_cb(del_btn, StageDeleteHandler, LV_EVENT_CLICKED, this);

  _edit_ctx->stage_cards.push_back(card);
  return card;
}

lv_obj_t* ProfilesScreen::CreateStageEditPage() {
  if (_edit_ctx->stage_page) {
    lv_obj_delete(_edit_ctx->stage_page);
    _edit_ctx->stage_page = nullptr;
  }
  _edit_ctx->stage_page = ui::CreateMenuPage(_labels->menu, "Edit Stage");
  // _labels->stage_edit_page = ui::CreateMenuPage(_labels->menu, "Edit Stage");
  lv_obj_t* section = ui::CreateMenuSection(_edit_ctx->stage_page);

  // Profile name input
  lv_obj_t* name_card = ui::CreateCard(section);
  lv_obj_set_width(name_card, lv_pct(100));
  lv_obj_set_height(name_card, LV_SIZE_CONTENT);

  ui::CreateBodyText(name_card, "Name:");
  _edit_ctx->stage_name_ta = ui::CreateTextArea(name_card);  // lv_textarea_create(name_card);
  lv_obj_add_event_cb(_edit_ctx->stage_name_ta, StageSaveHandler, LV_EVENT_READY, this);

  // lv_obj_set_width(_edit_ctx->stage_name_ta, lv_pct(100));
  lv_obj_set_flex_grow(_edit_ctx->stage_name_ta, 1);

  lv_textarea_set_one_line(_edit_ctx->stage_name_ta, true);
  lv_textarea_set_max_length(_edit_ctx->stage_name_ta, 32);

  // Add keyboard event handler
  lv_obj_add_event_cb(_edit_ctx->stage_name_ta, TextAreaEventHandler, LV_EVENT_FOCUSED, this);
  lv_obj_add_event_cb(_edit_ctx->stage_name_ta, TextAreaEventHandler, LV_EVENT_DEFOCUSED, this);
  lv_obj_add_event_cb(_edit_ctx->stage_name_ta, TextAreaEventHandler, LV_EVENT_READY, this);

  section = ui::CreateMenuSection(_edit_ctx->stage_page);
  lv_obj_set_width(section, lv_pct(100));
  lv_obj_set_flex_grow(section, 1);
  lv_obj_add_flag(section, LV_OBJ_FLAG_SCROLLABLE);

  // Disable overscroll/bounce
  lv_obj_set_scroll_dir(section, LV_DIR_VER);               // Only vertical scroll
  lv_obj_remove_flag(section, LV_OBJ_FLAG_SCROLL_ELASTIC);  // No bounce/elastic

  lv_obj_t* card = ui::CreateCard(section);
  lv_obj_set_width(card, lv_pct(100));
  lv_obj_set_height(card, LV_SIZE_CONTENT);
  // lv_obj_set_style_border_width(card, 3, 0);
  // lv_obj_set_style_border_color(card, lv_color_hex(0x008800), 0);
  // lv_obj_add_flag(card, LV_OBJ_FLAG_SCROLLABLE);

  // lv_obj_t* col = ui::CreateColumnContainer(card);
  // lv_obj_set_width(col, lv_pct(100));
  // lv_obj_set_height(col, LV_SIZE_CONTENT);
  // lv_obj_set_style_border_width(col, 3, 0);
  // lv_obj_set_style_border_color(col, lv_color_hex(0x880088), 0);

  lv_obj_t* row = ui::CreateRowContainer(card);
  lv_obj_set_style_flex_main_place(row, LV_FLEX_ALIGN_SPACE_BETWEEN, 0);

  // lv_obj_set_style_flex_main_place(fromrow, LV_FLEX_ALIGN_START, 0);
  lv_obj_t* col;

  lv_obj_set_width(row, lv_pct(100));
  lv_obj_set_height(row, LV_SIZE_CONTENT);  // LV_DPX(100));
  // lv_obj_set_flex_grow(fromrow, 1);  // Fill remaining space
  // lv_obj_set_style_border_width(row, 3, 0);
  // lv_obj_set_style_border_color(row, lv_color_hex(0x880000), 0);

  col = ui::CreateLabeledUnit(row, "From", 26.0f, "%.0f", "°C");
  lv_label_set_text(col, _edit_ctx->stage ? std::to_string((int)_edit_ctx->stage->start_temp).c_str() : "25");
  lv_obj_set_user_data(col, (void*)"from");
  lv_obj_add_flag(col, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_add_event_cb(col, StageEditUnitHandler, LV_EVENT_CLICKED, this);

  // ui::CreateBodyText(row, LV_SYMBOL_RIGHT);

  col = ui::CreateLabeledUnit(row, "To", 180.0f, "%.0f", "°C");
  lv_label_set_text(col, _edit_ctx->stage ? std::to_string((int)_edit_ctx->stage->end_temp).c_str() : "180");
  lv_obj_set_user_data(col, (void*)"to");
  lv_obj_add_flag(col, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_add_event_cb(col, StageEditUnitHandler, LV_EVENT_CLICKED, this);

  // ui::CreateBodyText(row, "/");

  col = ui::CreateLabeledUnit(row, "Over", 90.0f, "%.0f", "s");
  lv_label_set_text(col, _edit_ctx->stage ? std::to_string(_edit_ctx->stage->duration_ms / 1000).c_str() : "90");
  lv_obj_set_user_data(col, (void*)"duration");
  lv_obj_add_flag(col, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_add_event_cb(col, StageEditUnitHandler, LV_EVENT_CLICKED, this);
  // ui::CreateSmallText(row, "From:");
  // ui::CreateValueSmall(row, 25, "%d");
  // ui::CreateUnitLabel(row, "°C");
  // ui::CreateSmallText(row, LV_SYMBOL_RIGHT);
  // ui::CreateValueSmall(row, 180, "%d");
  // ui::CreateUnitLabel(row, "°C");
  // ui::CreateSmallText(row, "/");
  // ui::CreateValueSmall(row, 180, "%d");
  // ui::CreateSmallText(row, "seconds");

  // // Container for stage cards (will be populated later)
  // _labels->stages_list_container = ui::CreateColumnContainer(section);
  // lv_obj_set_width(_labels->stages_list_container, lv_pct(100));
  // lv_obj_set_height(_labels->stages_list_container, LV_SIZE_CONTENT);

  return _edit_ctx->stage_page;
}

void ProfilesScreen::PopulateStageEditPage(size_t index) {
  if (!_edit_ctx->profile) return;
  // if (!stage) return;

  // Set name
  const Profile::Stage* stage = _edit_ctx->profile->GetStage(index);  // Just to avoid unused variable warning
  if (_edit_ctx->stage_name_ta && stage) lv_textarea_set_text(_edit_ctx->stage_name_ta, stage->name.c_str());

  // Clear existing stage cards
  // lv_obj_clean(_labels->stages_list_container);
  // _edit_ctx->stage_cards.clear();
}

// ============================================================================
// Event Handlers
// ============================================================================

void ProfilesScreen::MenuBackHandler(lv_event_t* e) {
  lv_obj_t* obj = (lv_obj_t*)lv_event_get_target(e);
  ProfilesScreen* screen = (ProfilesScreen*)lv_event_get_user_data(e);
  // lv_event_get_current_target(e);
  // lv_obj_delete(obj);

  if (lv_menu_back_button_is_root(screen->_labels->menu, obj)) {
    FLOG_INFO("Back to main screen");
    PS_PUB_NIL("ui.action.return");
  }
}

void ProfilesScreen::ProfileSelectHandler(lv_event_t* e) {
  ProfilesScreen* screen = (ProfilesScreen*)lv_event_get_user_data(e);
  lv_obj_t* btn = (lv_obj_t*)lv_event_get_target(e);

  const char* name = (const char*)lv_obj_get_user_data(btn);
  screen->_selected_profile = name;

  FLOG_INFO("Selected profile: %s", name);

  // Update config to use this profile
  PS_PUB_STR("config.heater.profile.set", name);
}

void ProfilesScreen::ProfileEditHandler(lv_event_t* e) {
  ProfilesScreen* screen = (ProfilesScreen*)lv_event_get_user_data(e);
  lv_obj_t* btn = (lv_obj_t*)lv_event_get_target(e);

  const char* name = (const char*)lv_obj_get_user_data(btn);
  screen->_edit_ctx->profile = screen->_profile_mgr->GetProfile(name);
  screen->_edit_ctx->profile_original_name = name;  // Save original name

  if (!screen->_edit_ctx->profile) {
    FLOG_ERROR("Profile '%s' not found", name);
    return;
  }

  FLOG_INFO("Editing profile: %s", name);

  screen->CreateProfileEditPage();
  screen->PopulateProfileEditPage();
  // lv_menu_set_page(screen->_labels->menu, screen->_edit_ctx->profile_page);
}

void ProfilesScreen::ProfileDeleteHandler(lv_event_t* e) {
  ProfilesScreen* screen = (ProfilesScreen*)lv_event_get_user_data(e);
  lv_obj_t* btn = (lv_obj_t*)lv_event_get_target(e);

  const char* name = (const char*)lv_obj_get_user_data(btn);

  FLOG_INFO("Deleting profile: %s", name);

  ConfirmationContext ctx{.parent_screen = screen->_screen,
                          .backdrop = nullptr,
                          // .object = label,
                          .title = std::format("Delete {}?", name).c_str(),
                          .message = "Are you sure you want to delete this profile??",
                          .confirm_text = "Delete!",
                          .cancel_text = "Cancel",
                          .on_confirm =
                              [screen, name](void* obj) {
                                ConfirmationState* state = static_cast<ConfirmationState*>(obj);
                                FLOG_DEBUG("Deleting profile");
                                esp_err_t err = screen->_profile_mgr->DeleteProfile(name);
                                if (err == ESP_OK) {
                                  screen->RefreshProfileList();
                                } else {
                                  FLOG_ERROR("Failed to delete profile '%s'", name);
                                }
                                // if (state->backdrop) lv_obj_delete(state->backdrop);
                              },
                          .on_cancel =
                              [](void* obj) {
                                ConfirmationState* state = static_cast<ConfirmationState*>(obj);
                                FLOG_DEBUG("Cancelled Deleting profile");
                                // if (state->backdrop) lv_obj_delete(state->backdrop);
                              }};
  ConfirmationPopup(ctx);
}

void ProfilesScreen::ProfileLoadHandler(lv_event_t* e) {
  ProfilesScreen* screen = (ProfilesScreen*)lv_event_get_user_data(e);
  lv_obj_t* btn = (lv_obj_t*)lv_event_get_target(e);

  const char* name = (const char*)lv_obj_get_user_data(btn);
  auto profile = screen->_profile_mgr->GetProfile(name);

  if (!profile) {
    FLOG_ERROR("Profile '%s' not found", name);
    return;
  }

  // std::string(topics::heater::profile + ".set").c_str()
  PS_PUB_STR("heater.profile.set", name);  // TODO: template topics
  PS_PUB_NIL("ui.action.return");
}

void ProfilesScreen::ProfileAddHandler(lv_event_t* e) {
  ProfilesScreen* screen = (ProfilesScreen*)lv_event_get_user_data(e);

  FLOG_INFO("Adding new profile");

  screen->_edit_ctx->profile = std::make_shared<Profile>();
  screen->_edit_ctx->profile_original_name = "";  // Empty = new profile
  screen->_profile_mgr->SaveProfile(screen->_edit_ctx->profile->Name(), screen->_edit_ctx->profile);

  screen->CreateProfileEditPage();
  screen->PopulateProfileEditPage();
  // lv_menu_set_page(screen->_labels->menu, screen->_edit_ctx->profile_page);
}

void ProfilesScreen::ProfileSaveHandler(lv_event_t* e) {
  ProfilesScreen* screen = (ProfilesScreen*)lv_event_get_user_data(e);
  lv_event_code_t code = lv_event_get_code(e);

  // switch (code) {
  //   case LV_EVENT_READY:
  //     break;
  //   default:
  //     break;
  // }
  FLOG_DEBUG("Event code: %d", code);

  if (!screen->_edit_ctx->profile) return;
  if (code != LV_EVENT_READY) return;

  const char* name = lv_textarea_get_text(screen->_edit_ctx->profile_name_ta);
  if (!name || strlen(name) == 0) {
    FLOG_ERROR("Profile name is empty");
    return;
  }

  FLOG_INFO("Saving profile: %s", name);

  // If name changed and we were editing, delete old profile
  if (!screen->_edit_ctx->profile_original_name.empty() && screen->_edit_ctx->profile_original_name != name) {
    FLOG_INFO("Profile renamed from '%s' to '%s', deleting old", screen->_edit_ctx->profile_original_name.c_str(),
              name);
    screen->_profile_mgr->DeleteProfile(screen->_edit_ctx->profile_original_name);
  }

  screen->_profile_mgr->SaveProfile(name, screen->_edit_ctx->profile);

  // Go back to list
  // lv_menu_set_page(screen->_labels->menu, screen->_labels->root_page);
  screen->RefreshProfileList();
}

void ProfilesScreen::StageAddHandler(lv_event_t* e) {
  ProfilesScreen* screen = (ProfilesScreen*)lv_event_get_user_data(e);

  if (!screen->_edit_ctx->profile) return;

  // Add default stage
  Profile::Stage new_stage{25.0f, 100.0f, 60000, Profile::Shape::Linear, "New Stage"};
  screen->_edit_ctx->profile->AddStage(new_stage);
  // screen->_edit_ctx->stage = new_stage;

  screen->RefreshProfileList();
  screen->RefreshStageList();
}

void ProfilesScreen::StageSaveHandler(lv_event_t* e) {
  ProfilesScreen* screen = (ProfilesScreen*)lv_event_get_user_data(e);

  if (!screen->_edit_ctx->profile) return;

  if (!screen->_edit_ctx->stage) {
    FLOG_ERROR("No stage is being edited, can't save");
    return;
  }

  const char* name = lv_textarea_get_text(screen->_edit_ctx->stage_name_ta);
  if (!name || strlen(name) == 0) {
    FLOG_ERROR("Stage name is empty");
    return;
  }

  FLOG_INFO("Saving stage: %s", name);

  screen->_edit_ctx->stage->name = name;
  screen->_edit_ctx->profile->SaveStage(screen->_edit_ctx->stage_index, *(screen->_edit_ctx->stage));
  screen->_profile_mgr->SaveProfile(screen->_edit_ctx->profile->Name(), screen->_edit_ctx->profile);

  // If name changed and we were editing, delete old profile
  // if (!screen->_edit_ctx->profile_original_name.empty() && screen->_edit_ctx->profile_original_name != name) {
  //   FLOG_INFO("Profile renamed from '%s' to '%s', deleting old", screen->_edit_ctx->profile_original_name.c_str(),
  //   name); screen->_profile_mgr->DeleteProfile(screen->_edit_ctx->profile_original_name);
  // }

  // screen->_profile_mgr->SaveProfile(name, screen->_edit_ctx->profile);
  // screen->_edit_ctx->stage = name;
  // screen->_edit_ctx->profile->SaveStage(screen->_edit_ctx->stage_index, screen->_edit_ctx->stage);
  // screen->_edit_ctx->stage->

  // Go back to list
  // lv_menu_set_page(screen->_labels->menu, screen->_labels->root_page);
  screen->RefreshStageList();
  screen->RefreshProfileList();
}

void ProfilesScreen::StageEditHandler(lv_event_t* e) {
  ProfilesScreen* screen = (ProfilesScreen*)lv_event_get_user_data(e);
  lv_obj_t* btn = (lv_obj_t*)lv_event_get_target(e);

  size_t index = (size_t)lv_obj_get_user_data(btn);

  if (!screen->_edit_ctx->profile) {
    FLOG_ERROR("No profile is being edited, can't edit stage");
    return;
  }

  screen->_edit_ctx->stage = screen->_edit_ctx->profile->GetStage(index);
  if (!screen->_edit_ctx->stage) return;
  FLOG_INFO("Editing stage %zu: %s", index, screen->_edit_ctx->stage->name.c_str());

  screen->CreateStageEditPage();
  screen->PopulateStageEditPage(index);

  lv_menu_set_page(screen->_labels->menu, screen->_edit_ctx->stage_page);
}

void ProfilesScreen::StageEditUnitHandler(lv_event_t* e) {
  FLOG_INFO("StageEditUnitHandler called");
  ProfilesScreen* obj = (ProfilesScreen*)lv_event_get_user_data(e);
  lv_obj_t* label = (lv_obj_t*)lv_event_get_target(e);

  std::string target = (const char*)lv_obj_get_user_data(label);

  if (target.empty()) return;

  NumpadContext ctx{
      .parent_screen = obj->GetScreen(),
      .backdrop = nullptr,                        // backdrop
      .target_spinbox = obj->_edit_ctx->spinbox,  // spinbox
  };

  if (target.compare("from") == 0) {
    FLOG_INFO("Editing 'from' temperature");
    ctx.on_confirm = [obj, label](std::optional<int32_t> value) {
      if (!value.has_value()) return;
      FLOG_INFO("New 'from' temperature: %li", value.value());
      lv_label_set_text(label, std::to_string(value.value()).c_str());
      if (obj->_edit_ctx->stage) {
        obj->_edit_ctx->stage->start_temp = value.value();
        obj->_edit_ctx->profile->SaveStage(obj->_edit_ctx->stage_index, *(obj->_edit_ctx->stage));
        obj->_profile_mgr->SaveProfile(obj->_edit_ctx->profile->Name(), obj->_edit_ctx->profile);
        obj->RefreshStageList();
        obj->RefreshProfileList();
      }
    };
    ctx.initial_value = obj->_edit_ctx->stage ? (int32_t)obj->_edit_ctx->stage->start_temp : 25;

  } else if (target.compare("to") == 0) {
    FLOG_INFO("Editing 'to' temperature");
    ctx.on_confirm = [obj, label](std::optional<int32_t> value) {
      if (!value.has_value()) return;
      FLOG_INFO("New 'to' temperature: %li", value.value());
      lv_label_set_text(label, std::to_string(value.value()).c_str());
      if (obj->_edit_ctx->stage) {
        obj->_edit_ctx->stage->end_temp = value.value();
        obj->_edit_ctx->profile->SaveStage(obj->_edit_ctx->stage_index, *(obj->_edit_ctx->stage));

        obj->RefreshStageList();
        obj->RefreshProfileList();
      }
    };
    ctx.initial_value = obj->_edit_ctx->stage ? (int32_t)obj->_edit_ctx->stage->end_temp : 180;
  } else if (target.compare("duration") == 0) {
    FLOG_INFO("Editing 'duration'");
    ctx.on_confirm = [obj, label](std::optional<int32_t> value) {
      if (!value.has_value()) return;
      FLOG_INFO("New 'duration': %li", value.value());
      lv_label_set_text(label, std::to_string(value.value()).c_str());
      if (obj->_edit_ctx->stage) {
        obj->_edit_ctx->stage->duration_ms = value.value() * 1000;
        obj->_edit_ctx->profile->SaveStage(obj->_edit_ctx->stage_index, *(obj->_edit_ctx->stage));
        obj->_edit_ctx->profile->SaveStage(obj->_edit_ctx->stage_index, *(obj->_edit_ctx->stage));

        obj->RefreshStageList();
        obj->RefreshProfileList();
      }
    };
    ctx.initial_value = obj->_edit_ctx->stage ? (int32_t)(obj->_edit_ctx->stage->duration_ms / 1000) : 90;
  }

  NumpadOpen(ctx);
}

void ProfilesScreen::StageDeleteHandler(lv_event_t* e) {
  ProfilesScreen* screen = (ProfilesScreen*)lv_event_get_user_data(e);
  lv_obj_t* btn = (lv_obj_t*)lv_event_get_target(e);

  size_t index = (size_t)lv_obj_get_user_data(btn);

  if (!screen->_edit_ctx->profile) return;
  const std::string& name = screen->_edit_ctx->profile->GetStage(index)->name;

  ConfirmationContext ctx{.parent_screen = screen->_screen,
                          .backdrop = nullptr,
                          // .object = label,
                          .title = std::format("Delete {}?", name).c_str(),
                          .message = "Are you sure you want to delete this stage??",
                          .confirm_text = "Delete!",
                          .cancel_text = "Cancel",
                          .on_confirm =
                              [screen, index](void* obj) {
                                ConfirmationState* state = static_cast<ConfirmationState*>(obj);
                                FLOG_DEBUG("Deleting Stage");
                                // esp_err_t err = screen->_profile_mgr->DeleteProfile(name);
                                esp_err_t err = screen->_edit_ctx->profile->RemoveStage(index);
                                if (err == ESP_OK) {
                                  screen->RefreshStageList();
                                  screen->RefreshProfileList();
                                } else {
                                  FLOG_ERROR("Failed to delete stage");
                                }
                                // if (state->backdrop) lv_obj_delete(state->backdrop);
                              },
                          .on_cancel =
                              [](void* obj) {
                                ConfirmationState* state = static_cast<ConfirmationState*>(obj);
                                FLOG_DEBUG("Cancelled Deleting profile");
                                // if (state->backdrop) lv_obj_delete(state->backdrop);
                              }};
  ConfirmationPopup(ctx);

  // FLOG_INFO("Deleting stage %zu", index);

  // screen->_edit_ctx->profile->RemoveStage(index);
  // screen->PopulateProfileEditPage();
}

// TODO: centralize this

void ProfilesScreen::TextAreaEventHandler(lv_event_t* e) {
  ProfilesScreen* screen = (ProfilesScreen*)lv_event_get_user_data(e);
  lv_obj_t* ta = (lv_obj_t*)lv_event_get_target(e);
  lv_event_code_t code = lv_event_get_code(e);

  if (code == LV_EVENT_FOCUSED) {
    // Create keyboard if it doesn't exist
    if (!screen->_labels->keyboard) {
      screen->_labels->keyboard = lv_keyboard_create(lv_screen_active());
      lv_obj_set_size(screen->_labels->keyboard, lv_pct(100), lv_pct(60));

      // Position at bottom using align
      lv_obj_align(screen->_labels->keyboard, LV_ALIGN_BOTTOM_MID, 0, 0);

      // Make it floating (ignores parent layout)
      lv_obj_add_flag(screen->_labels->keyboard, LV_OBJ_FLAG_FLOATING);
    }
    lv_keyboard_set_textarea(screen->_labels->keyboard, ta);
    lv_obj_remove_flag(screen->_labels->keyboard, LV_OBJ_FLAG_HIDDEN);
    lv_obj_move_foreground(screen->_labels->keyboard);
  } else if (code == LV_EVENT_DEFOCUSED) {
    if (screen->_labels->keyboard) {
      lv_obj_add_flag(screen->_labels->keyboard, LV_OBJ_FLAG_HIDDEN);
    }
  } else if (code == LV_EVENT_READY) {
    // User pressed "OK" button on keyboard
    FLOG_INFO("Keyboard OK pressed, text: %s", lv_textarea_get_text(ta));
    // Hide keyboard
    if (screen->_labels->keyboard) {
      lv_obj_add_flag(screen->_labels->keyboard, LV_OBJ_FLAG_HIDDEN);
    }
  }
}

}  // namespace toothless