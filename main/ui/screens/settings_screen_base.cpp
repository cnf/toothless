// settings_screen.cpp
#include <esp_app_desc.h>
#include <esp_chip_info.h>
#include <esp_clk_tree.h>
#include <esp_flash.h>
#include <esp_psram.h>
#include <esp_system.h>

#include <format>
#include <variant>

#include "helpers.hpp"
#include "settings_screen.hpp"
#include "ui/themes/widget_factories.hpp"
#include "ui/user_interface.hpp"

extern "C" {
#include <pubsub.h>
}

namespace toothless {

esp_err_t SettingsScreen::CreateMenu() {
  FLOG_DEBUG("Creating Settings Menu");
  _labels->menu = ui::CreateMenu(_screen, "Settings");
  // lv_obj_set_flex_grow(_labels->menu, 1);
  {
    // Back button
    lv_menu_set_mode_root_back_button(_labels->menu, LV_MENU_ROOT_BACK_BUTTON_ENABLED);
    lv_obj_add_event_cb(_labels->menu, MenuBackEventHandler, LV_EVENT_CLICKED, this);
    lv_obj_t* back_btn = lv_menu_get_main_header_back_button(_labels->menu);
    lv_obj_set_ext_click_area(back_btn, lv_pct(33));
    lv_obj_set_width(back_btn, lv_pct(10));
  }
  /*Create a index page*/
  _labels->root_page = ui::CreateMenuRootPage(_labels->menu, NULL);
  _labels->section = ui::CreateMenuRootSection(_labels->root_page);

  // {
  //   auto values = std::make_shared<SettingsMap>();
  //   GetSettings(values, "ui");
  //   BuildFromEntries(_labels->menu, "ui", ui_config_entries, *values, "User Interface");
  // }
  // {
  //   auto values = std::make_shared<SettingsMap>();
  //   GetSettings(values, "heater");
  //   BuildFromEntries(_labels->menu, "heater", heater::config_entries, *values, "Heater Settings");
  // }
  BuildSettingsUI(_labels->menu, "ui", "User Interface");
  BuildSettingsUI(_labels->menu, "heater", "Hearer Settings");

  ui::CreateHeading(_labels->root_page, "Info");
  // lv_obj_t* cont = ui::CreateMenuRootEntry(_labels->section, "Info", LV_SYMBOL_LIST);

  _labels->section = ui::CreateMenuRootSection(_labels->root_page);

  CreateSubFirmwareInfo(_labels->menu, _labels->section);
  CreateSubSystemInfo(_labels->menu, _labels->section);
  lv_obj_t* sidebar_switch = ui::CreateSwitch(_labels->section, _labels->sidebar);
  ui::CreateMenuRootEntry(_labels->section, sidebar_switch, LV_SYMBOL_SETTINGS);
  lv_obj_add_event_cb(sidebar_switch, SidebarHandler, LV_EVENT_VALUE_CHANGED, this);

  // SetSidebar(_labels->sidebar);

  lv_menu_set_page(_labels->menu, _labels->root_page);

  // ui::StyleMenuSidebar(_labels->menu);

  FLOG_DEBUG("Menu Created");
  return ESP_OK;
}

esp_err_t SettingsScreen::SetSidebar(bool mode) {
  FLOG_DEBUG("Toggling menu mode");
  if (mode) {
    _labels->sidebar = true;
    lv_menu_set_page(_labels->menu, NULL);
    lv_menu_set_sidebar_page(_labels->menu, _labels->root_page);
    lv_obj_send_event(lv_obj_get_child(lv_obj_get_child(lv_menu_get_cur_sidebar_page(_labels->menu), 0), 0),
                      LV_EVENT_CLICKED, NULL);
    ui::StyleMenuSidebar(_labels->menu);
    // lv_menu_get_sidebar_header_back_button(_labels->menu);
    //   {
    //     goto nosidebarheader;
    //     lv_obj_t* sidebar_header = lv_menu_get_sidebar_header(_labels->menu);
    //     lv_obj_add_flag(sidebar_header, LV_OBJ_FLAG_CLICKABLE);
    //     // lv_obj_add_event_cb(sidebar_header, MenuBackEventHandler, LV_EVENT_CLICKED, this); <-- already set in
    //     // CreateMenu
    //     lv_obj_set_style_pad_left(sidebar_header, 30, 0);  // increases clickable area
    //     lv_obj_set_ext_click_area(sidebar_header, 10);
    //     lv_obj_set_style_border_width(sidebar_header, 0, 0);
    //     // lv_obj_set_width(sidebar_header, lv_pct(10));
    //   }
    // nosidebarheader:;
    //   {
    //     goto nobackbutton;
    //     lv_obj_t* sidebar_back = lv_menu_get_sidebar_header_back_button(_labels->menu);
    //     lv_obj_add_flag(sidebar_back, LV_OBJ_FLAG_CLICKABLE);
    //     // lv_obj_set_ext_click_area(sidebar_back, 10);  // enlarge hitbox around the label
    //     lv_obj_set_ext_click_area(sidebar_back, lv_pct(33));
    //     lv_obj_set_width(sidebar_back, lv_pct(10));
    //     lv_obj_set_style_pad_right(sidebar_back, 30, 0);  // increases clickable area

    //     // lv_obj_add_event_cb(sidebar_back, MenuBackEventHandler, LV_EVENT_CLICKED, this);
    //   }
    // nobackbutton:;

  } else {
    _labels->sidebar = false;
    lv_menu_set_sidebar_page(_labels->menu, NULL);
    lv_menu_clear_history(_labels->menu); /* Clear history because we will be showing the root page later */
    lv_menu_set_page(_labels->menu, _labels->root_page);
  }
  return ESP_OK;
}

void SettingsScreen::BuildSettingsUI(lv_obj_t* parent, const char* namespace_name, const char* title) {
  // esp_log_level_set(FLOG_SHORT_FILENAME, ESP_LOG_DEBUG);
  FLOG_INFO("Building settings UI for namespace: %s", namespace_name);

  auto values = std::make_shared<SettingsMap>();
  GetSettings(values, namespace_name);
  ConfigEntries ui_entries = GetConfigEntries(namespace_name);
  BuildFromEntries(_labels->menu, namespace_name, ui_entries, *values, title);

  // Fetch entries
  // const ConfigEntries* entries = GetConfigEntries(namespace_name);
  // if (!entries) {
  //   FLOG_ERROR("No entries for %s", namespace_name);
  //   return;
  // }

  // Fetch current values
  // auto values = std::make_shared<SettingsMap>();
  // GetSettings(values, namespace_name);

  // Build UI
  // SettingsScreen screen;
  // BuildFromEntries(parent, namespace_name, ui_config_entries, *values, "User Interface");
}

void SettingsScreen::BuildFromEntries(lv_obj_t* parent, const char* namespace_name, const ConfigEntries& entries,
                                      const SettingsMap& current_values, std::string title) {
  for (const auto& entry : entries) {
    FLOG_INFO("BuildFromEntries sees: key=%s, type=%d, format='%s'", entry.key, entry.type, entry.format.c_str());
  }
  // Create main container
  if (title.empty()) {
    title = ui::SnakeToTitle(std::string(namespace_name));
  }
  lv_obj_t* sub_page = ui::CreateMenuPage(_labels->menu, title.c_str());
  // lv_menu_separator_create(sub_page);
  lv_obj_t* container = ui::CreateMenuSection(sub_page);

  for (const auto& entry : entries) {
    // Create card for each setting
    lv_obj_t* card = ui::CreateCard(container);
    lv_obj_set_width(card, lv_pct(100));
    lv_obj_set_height(card, LV_SIZE_CONTENT);

    // lv_obj_set_style_pad_all(card, 12, 0);

    lv_obj_t* desc_label = ui::CreateBodyText(card, entry.description.c_str());
    lv_obj_set_size(desc_label, lv_pct(50), LV_SIZE_CONTENT);

    lv_obj_t* col = ui::CreateColumnContainer(card);
    lv_obj_set_size(col, lv_pct(50), LV_SIZE_CONTENT);
    lv_obj_set_style_pad_all(col, 5, 0);
    lv_obj_set_style_pad_gap(col, 10, 0);

    // Right: widget based on type
    lv_obj_t* widget = nullptr;

    // Get current value
    auto it = current_values.find(entry.key);
    if (it == current_values.end()) continue;  // Skip if no value

    switch (entry.type) {
      case ConfigValueTypes::kBool: {
        bool val = std::get<bool>(it->second);
        widget = BuildBoolSetting(col, entry, val);
        break;
      }
      case ConfigValueTypes::kInt: {
        int val = std::get<int>(it->second);
        widget = BuildIntSetting(col, entry, val);
        break;
      }
      case ConfigValueTypes::kDouble: {
        double val = std::get<double>(it->second);
        widget = BuildDoubleSetting(col, entry, val);
        break;
      }
      case ConfigValueTypes::kString: {
        std::string val = std::get<std::string>(it->second);
        // Check if enum
        if (entry.format.find("enum=") != std::string::npos) {
          widget = BuildEnumSetting(col, entry, val);
        } else {
          widget = BuildStringSetting(col, entry, val);
        }
        break;
      }
      default:
        continue;
    }

    // Store metadata for callbacks
    if (widget) {
      auto* data = new WidgetData{namespace_name, entry.key, entry.type};
      _widget_map[widget] = data;
    }
  }
  lv_obj_t* cont = ui::CreateMenuRootEntry(_labels->section, ui::SnakeToTitle(namespace_name).c_str(), LV_SYMBOL_LIST);
  lv_menu_set_load_page_event(_labels->menu, cont, sub_page);
}

lv_obj_t* SettingsScreen::CreateSubFirmwareInfo(lv_obj_t* parent, lv_obj_t* root) {
  const esp_app_desc_t* desc = esp_app_get_description();
  lv_obj_t* page = ui::CreateMenuPage(parent, "Firmware");
  lv_obj_t* section = ui::CreateMenuSection(page);
  lv_obj_t* wrapper = ui::CreateColumnContainer(section);
  lv_obj_set_flex_flow(wrapper, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(wrapper, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
  lv_obj_set_width(wrapper, lv_pct(100));
  lv_obj_set_height(wrapper, LV_SIZE_CONTENT);

  char lvgl_version[10];
  sprintf(lvgl_version, "%d.%d.%d", LVGL_VERSION_MAJOR, LVGL_VERSION_MINOR, LVGL_VERSION_PATCH);

  ui::CreateIconItem(wrapper, "Toothless is an open-source reflow oven controller firmware.", LV_SYMBOL_BULLET);
  // ui::CreateIconItem(wrapper, std::format("Firmware: {}", desc->project_name).c_str(), LV_SYMBOL_BULLET);
  ui::CreateIconItem(wrapper, std::format("Version: {}", desc->version).c_str(), LV_SYMBOL_BULLET);
  ui::CreateIconItem(wrapper, std::format("ESP-IDF: {}", desc->idf_ver).c_str(), LV_SYMBOL_BULLET);
  ui::CreateIconItem(wrapper, std::format("LVGL: {}", lvgl_version).c_str(), LV_SYMBOL_BULLET);
  // ui::CreateIconItem(wrapper, std::format("Build Date: {}", __DATE__).c_str(),
  //  LV_SYMBOL_BULLET);  // BUG: for some reason, i can not get an actual date here... once compiled it
  // is always "Jan  1 1980"
  ui::CreateIconItem(wrapper, "https://github.com/cnf/Toothless", LV_SYMBOL_HOME);

  lv_obj_t* cont = ui::CreateMenuRootEntry(root, "About", NULL);

  lv_menu_set_load_page_event(parent, cont, page);

  return page;
}

lv_obj_t* SettingsScreen::CreateSubSystemInfo(lv_obj_t* parent, lv_obj_t* root) {
  const esp_app_desc_t* desc = esp_app_get_description();
  lv_obj_t* page = ui::CreateMenuPage(parent, "System");
  lv_obj_t* section = ui::CreateMenuSection(page);
  lv_obj_t* wrapper = ui::CreateColumnContainer(section);
  lv_obj_set_flex_flow(wrapper, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(wrapper, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
  lv_obj_set_width(wrapper, lv_pct(100));
  lv_obj_set_height(wrapper, LV_SIZE_CONTENT);

  ChipInfo chip_info;
  GetChipInfo(chip_info);
  // esp_chip_info_t chip_info;
  // esp_chip_info(&chip_info);
  // uint32_t hz;
  // esp_clk_tree_src_get_freq_hz(CLK_SRC_CPU, ESP_CLK_TREE_SRC_FREQ_PRECISION_EXACT, &hz);

  uint32_t flash_size = 0;
  esp_err_t err = esp_flash_get_size(NULL, &flash_size);  // NULL = default flash
  ui::CreateHeading(wrapper, "Chip Info");
  ui::CreateIconItem(wrapper, std::format("Model: {} Rev {}", chip_info.model, chip_info.revision).c_str(),
                     LV_SYMBOL_BULLET);
  ui::CreateIconItem(wrapper,
                     std::format("{} Core(s) at {} MHz", chip_info.cores, CONFIG_ESP_DEFAULT_CPU_FREQ_MHZ).c_str(),
                     LV_SYMBOL_BULLET);
  ui::CreateIconItem(wrapper, std::format("Flash Size: {} MB", std::to_string(flash_size / (1024 * 1024))).c_str(),
                     LV_SYMBOL_BULLET);
#if defined(CONFIG_SPIRAM)
  if (esp_psram_is_initialized()) {
    size_t psram_size = esp_psram_get_size();
    ui::CreateIconItem(wrapper, std::format("PSRAM Size: {} MB", std::to_string(psram_size / (1024 * 1024))).c_str(),
                       LV_SYMBOL_BULLET);
  }
#endif

  ui::CreateHeading(wrapper, "Memory Info");
  ui::CreateTitle(wrapper, "Internal");

  uint32_t total_internal_memory = heap_caps_get_total_size(MALLOC_CAP_INTERNAL);
  uint32_t free_internal_memory = heap_caps_get_free_size(MALLOC_CAP_INTERNAL);
  uint32_t largest_contig_internal_block = heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL);
  ui::CreateIconItem(wrapper, std::format("Total DRAM: {} Kbytes", total_internal_memory / 1024).c_str(),
                     LV_SYMBOL_BULLET);
  ui::CreateIconItem(wrapper, std::format("Free DRAM: {} Kbytes", free_internal_memory / 1024).c_str(),
                     LV_SYMBOL_BULLET);
  ui::CreateIconItem(wrapper,
                     std::format("Largest Free Block: {} Kbytes", largest_contig_internal_block / 1024).c_str(),
                     LV_SYMBOL_BULLET);

#if defined(CONFIG_SPIRAM)
  if (esp_psram_is_initialized()) {
    ui::CreateTitle(wrapper, "PSRAM");
    total_internal_memory = heap_caps_get_total_size(MALLOC_CAP_SPIRAM);
    free_internal_memory = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
    largest_contig_internal_block = heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM);
    ui::CreateIconItem(wrapper, std::format("Total PSRAM: {} Kbytes", total_internal_memory / 1024).c_str(),
                       LV_SYMBOL_BULLET);
    ui::CreateIconItem(wrapper, std::format("Free PSRAM: {} Kbytes", free_internal_memory / 1024).c_str(),
                       LV_SYMBOL_BULLET);
    ui::CreateIconItem(
        wrapper, std::format("Largest Contiguous  Block: {} Kbytes", largest_contig_internal_block / 1024).c_str(),
        LV_SYMBOL_BULLET);
  }
#endif
  ui::CreateTitle(wrapper, "Totals");
  size_t free_heap = esp_get_free_heap_size();
  size_t largest_free_block = heap_caps_get_largest_free_block(MALLOC_CAP_8BIT);
  ui::CreateIconItem(wrapper, std::format("Free Heap: {} Kbytes", free_heap / 1024).c_str(), LV_SYMBOL_BULLET);
  ui::CreateIconItem(wrapper, std::format("Largest Free Block: {} Kbytes", largest_free_block / 1024).c_str(),
                     LV_SYMBOL_BULLET);

#if defined(CONFIG_LV_USE_BUILTIN_MALLOC)
  ui::CreateHeading(wrapper, "LVGL Info");
  lv_mem_monitor_t mem_mon;
  lv_mem_monitor(&mem_mon);
  ui::CreateIconItem(wrapper, std::format("Total Heap: {} bytes", mem_mon.total_size).c_str(), LV_SYMBOL_BULLET);
  ui::CreateIconItem(wrapper, std::format("Free Heap: {} bytes", mem_mon.free_size).c_str(), LV_SYMBOL_BULLET);
  ui::CreateIconItem(wrapper, std::format("Largest Free Block: {} bytes", mem_mon.free_biggest_size).c_str(),
                     LV_SYMBOL_BULLET);
  ui::CreateIconItem(wrapper, std::format("Used Heap: {} bytes", mem_mon.max_used).c_str(), LV_SYMBOL_BULLET);
  ui::CreateIconItem(wrapper, std::format("Heap Usage: {}%", mem_mon.used_pct).c_str(), LV_SYMBOL_BULLET);
#endif

  // CreateCBButton(wrapper, "Reboot", false, ResetHandler, nullptr);
  lv_obj_t* btn = ui::CreatePrimaryButton(wrapper, "Reboot", lv_pct(100), NULL, false);
  lv_obj_add_event_cb(btn, ResetHandler, LV_EVENT_CLICKED, nullptr);

  lv_obj_t* cont = ui::CreateMenuRootEntry(root, "System", NULL);

  lv_menu_set_load_page_event(parent, cont, page);

  return page;
}

lv_obj_t* SettingsScreen::BuildBoolSetting(lv_obj_t* parent, const ConfigEntry& entry, bool current_value) {
  lv_obj_t* sw = ui::CreateSwitch(parent, current_value);
  lv_obj_add_event_cb(sw, OnSwitchChanged, LV_EVENT_VALUE_CHANGED, this);
  return sw;
}

lv_obj_t* SettingsScreen::BuildIntSetting(lv_obj_t* parent, const ConfigEntry& entry, int current_value) {
  auto validator = ParseValidatorFromFormat(entry.format);
  FLOG_ERROR("Int setting: min=%f, max=%f, has_min=%s, has_max=%s, value=%li", validator.min_val, validator.max_val,
             validator.has_min ? "true" : "false", validator.has_max ? "true" : "false", current_value);

  lv_obj_t* wrapper = ui::CreateRowContainer(parent);
  lv_obj_set_height(wrapper, LV_SIZE_CONTENT);
  lv_obj_set_width(wrapper, LV_SIZE_CONTENT);
  lv_obj_set_flex_align(wrapper, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

  lv_obj_t* val_label = ui::CreateValueSmall(wrapper, current_value, "%li");
  lv_label_set_text(val_label, std::to_string(current_value).c_str());
  if (!entry.unit.empty()) {
    ui::CreateBodyText(wrapper, entry.unit.c_str());
  }

  if (validator.has_min && validator.has_max) {
    // Use slider
    lv_obj_t* slider = ui::CreateSlider(parent, validator.min_val, validator.max_val, current_value);
    lv_obj_set_width(slider, lv_pct(100));
    lv_obj_add_event_cb(slider, OnSliderChanged, LV_EVENT_ALL, this);
    // Store label in user_data for updates
    lv_obj_set_user_data(slider, val_label);
    return slider;
  }
  return val_label;
}

lv_obj_t* SettingsScreen::BuildDoubleSetting(lv_obj_t* parent, const ConfigEntry& entry, double current_value) {
  auto validator = ParseValidatorFromFormat(entry.format);

  lv_obj_t* wrapper = ui::CreateRowContainer(parent);
  lv_obj_set_height(wrapper, LV_SIZE_CONTENT);
  lv_obj_set_width(wrapper, LV_SIZE_CONTENT);
  lv_obj_set_flex_align(wrapper, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

  // lv_obj_t* val_label = ui::CreateValueSmall(parent, current_value, "%.3f");
  lv_obj_t* val_label = ui::CreateBodyText(wrapper, std::format("{:.3f}", current_value).c_str());
  lv_obj_set_height(val_label, LV_SIZE_CONTENT);

  if (!entry.unit.empty()) {
    ui::CreateBodyText(wrapper, entry.unit.c_str());
  }

  if (validator.has_min && validator.has_max) {
    // FIXME: Don't use a slider here, as it only supports integers
    lv_obj_t* slider = ui::CreateSlider(parent, validator.min_val * 10, validator.max_val * 10, current_value * 10);
    // lv_obj_set_width(slider, 150);
    lv_obj_set_height(slider, LV_SIZE_CONTENT);

    lv_obj_add_event_cb(slider, OnSliderChanged, LV_EVENT_ALL, this);
    lv_obj_set_user_data(slider, val_label);
    return slider;
  }
  return val_label;
}

lv_obj_t* SettingsScreen::BuildEnumSetting(lv_obj_t* parent, const ConfigEntry& entry,
                                           const std::string& current_value) {
  auto enum_vals = ParseEnumFromFormat(entry.format);
  if (enum_vals.empty()) return nullptr;

  // Build options string and find selected index
  std::string opts;
  int selected_idx = 0;

  // Normalize current value for comparison
  std::string normalized_current = ui::TitleToSnake(current_value);

  for (size_t i = 0; i < enum_vals.size(); ++i) {
    if (i > 0) opts += "\n";
    opts += ui::SnakeToTitle(enum_vals[i]);

    // Compare normalized values
    std::string normalized_option = ui::TitleToSnake(enum_vals[i]);
    if (normalized_option == normalized_current) {
      selected_idx = i;
    }
  }

  lv_obj_t* dropdown = ui::CreateDropdown(parent, NULL, opts.c_str(), selected_idx);
  lv_obj_add_event_cb(dropdown, OnDropdownChanged, LV_EVENT_VALUE_CHANGED, this);
  lv_obj_set_width(dropdown, lv_pct(100));

  return dropdown;
}

// lv_obj_t* SettingsScreen::BuildEnumSetting(lv_obj_t* parent, const ConfigEntry& entry,
//                                            const std::string& current_value) {
//   auto enum_vals = ParseEnumFromFormat(entry.format);
//   if (enum_vals.empty()) return nullptr;
//   for (const auto& val : enum_vals) {
//     FLOG_INFO("Enum option: %s", val.c_str());
//   }

//   // Build options string
//   std::string opts;
//   // std::string title = current_value;
//   int selected_idx = 0;

//   for (size_t i = 0; i < enum_vals.size(); ++i) {
//     if (i > 0) opts += "\n";
//     opts += ui::SnakeToTitle(enum_vals[i]);
//     if (enum_vals[i] == current_value) selected_idx = i;
//   }

//   lv_obj_t* dropdown = ui::CreateDropdown(parent, NULL, opts.c_str(), selected_idx);
//   lv_obj_add_event_cb(dropdown, OnDropdownChanged, LV_EVENT_VALUE_CHANGED, this);
//   lv_obj_set_width(dropdown, lv_pct(100));

//   return dropdown;
// }

lv_obj_t* SettingsScreen::BuildStringSetting(lv_obj_t* parent, const ConfigEntry& entry,
                                             const std::string& current_value) {
  // For now, just show current value (could add text input)
  return ui::CreateBodyText(parent, current_value.c_str());
}

// ============================================================================
// Event Handlers
// ============================================================================

void SettingsScreen::OnSwitchChanged(lv_event_t* e) {
  auto* screen = (SettingsScreen*)lv_event_get_user_data(e);
  lv_obj_t* sw = static_cast<lv_obj_t*>(lv_event_get_target(e));

  auto it = screen->_widget_map.find(sw);
  if (it == screen->_widget_map.end()) return;

  WidgetData* data = it->second;
  bool val = lv_obj_has_state(sw, LV_STATE_CHECKED);

  char topic[128];
  snprintf(topic, sizeof(topic), "config.%s.%s.set", data->namespace_name.c_str(), data->key.c_str());
  PS_PUB_BOOL(topic, val);
}

void SettingsScreen::OnSliderChanged(lv_event_t* e) {
  lv_event_code_t code = lv_event_get_code(e);
  auto* screen = (SettingsScreen*)lv_event_get_user_data(e);
  lv_obj_t* slider = static_cast<lv_obj_t*>(lv_event_get_target(e));

  auto it = screen->_widget_map.find(slider);
  if (it == screen->_widget_map.end()) return;

  WidgetData* data = it->second;
  int32_t val = lv_slider_get_value(slider);

  char topic[128];
  lv_obj_t* label = nullptr;

  switch (code) {
    case LV_EVENT_VALUE_CHANGED:
      // Update attached label
      label = (lv_obj_t*)lv_obj_get_user_data(slider);
      if (label) {
        if (data->type == ConfigValueTypes::kDouble) {
          // ui::UpdateValueLabel(label, val, "%.1f");
          lv_label_set_text_fmt(label, "%.1f", val * 1.0f);
        } else {
          // ui::UpdateValueLabel(label, val, "%d");
          lv_label_set_text_fmt(label, "%li", val);
        }
      }
      break;
    case LV_EVENT_RELEASED:
      snprintf(topic, sizeof(topic), "config.%s.%s.set", data->namespace_name.c_str(), data->key.c_str());

      if (data->type == ConfigValueTypes::kDouble) {
        PS_PUB_DBL(topic, val);
      } else {
        PS_PUB_INT(topic, val);
      }
      break;
    default:
      FLOG_DEBUG("Unhandled slider event code: %d", code);
      break;
      // return;
  }
}

void SettingsScreen::OnRollerChanged(lv_event_t* e) {
  auto* screen = (SettingsScreen*)lv_event_get_user_data(e);
  lv_obj_t* roller = static_cast<lv_obj_t*>(lv_event_get_target(e));

  auto it = screen->_widget_map.find(roller);
  if (it == screen->_widget_map.end()) return;

  WidgetData* data = it->second;
  uint16_t idx = lv_roller_get_selected(roller);

  char buf[64];
  lv_roller_get_selected_str(roller, buf, sizeof(buf));

  char topic[128];
  snprintf(topic, sizeof(topic), "config.%s.%s.set", data->namespace_name.c_str(), data->key.c_str());
  PS_PUB_STR(topic, buf);
}

void SettingsScreen::OnDropdownChanged(lv_event_t* e) {
  auto* screen = (SettingsScreen*)lv_event_get_user_data(e);
  lv_obj_t* dropdown = static_cast<lv_obj_t*>(lv_event_get_target(e));

  auto it = screen->_widget_map.find(dropdown);
  if (it == screen->_widget_map.end()) return;

  WidgetData* data = it->second;
  uint16_t idx = lv_dropdown_get_selected(dropdown);

  char option[64];
  lv_dropdown_get_selected_str(dropdown, option, sizeof(option));
  // option = ui::TitleToSnake(std::string(option)).c_str();

  // const char* option = lv_dropdown_get_selected_str(dropdown);

  char topic[128];
  snprintf(topic, sizeof(topic), "config.%s.%s.set", data->namespace_name.c_str(), data->key.c_str());
  PS_PUB_STR(topic, ui::TitleToSnake(std::string(option)).c_str());
}

void SettingsScreen::SidebarHandler(lv_event_t* e) {
  lv_event_code_t code = lv_event_get_code(e);
  SettingsScreen* screen = (SettingsScreen*)lv_event_get_user_data(e);
  if (!screen) return;

  // lv_obj_t* menu = (lv_obj_t*)lv_event_get_user_data(e);
  FLOG_DEBUG("Toggling Sidebar");
  lv_obj_t* obj = (lv_obj_t*)lv_event_get_target(e);
  if (code == LV_EVENT_VALUE_CHANGED) {
    if (lv_obj_has_state(obj, LV_STATE_CHECKED)) {
      screen->SetSidebar(true);
    } else {
      screen->SetSidebar(false);
    }
  }
}

void SettingsScreen::MenuBackEventHandler(lv_event_t* e) {
  lv_obj_t* obj = (lv_obj_t*)lv_event_get_target(e);
  // lv_obj_t* menu = (lv_obj_t*)lv_event_get_user_data(e);
  SettingsScreen* screen = (SettingsScreen*)lv_event_get_user_data(e);

  if (lv_menu_back_button_is_root(screen->_labels->menu, obj)) {
    BackButtonHandler(e);
    //   lv_obj_t* mbox1 = lv_msgbox_create(NULL);
    //   lv_msgbox_add_title(mbox1, "Hello");
    //   lv_msgbox_add_text(mbox1, "Root back btn click.");
    //   lv_msgbox_add_close_button(mbox1);
  }
}

void SettingsScreen::NumpadOpenHandler(lv_event_t* e) {
  SettingsScreen* obj = (SettingsScreen*)lv_event_get_user_data(e);

  NumpadContext ctx{.parent_screen = obj->GetScreen(),
                    .backdrop = nullptr,                         // backdrop
                    .target_spinbox = obj->_labels->set_target,  // spinbox
                    .on_confirm = [obj](std::optional<int32_t> val) {
                      if (val.has_value() && !std::isnan(val.value())) {
                        FLOG_INFO("Value: %li (* 100)", val.value());
                        PS_PUB_INT("heater.target.temperature.set", val.value() * 100);
                      } else {
                        PS_PUB_NIL("heater.target.temperature.set");
                      }
                      // lv_label_set_text(obj->_labels->set_target, )
                    }};
  NumpadOpen(ctx);
}

void SettingsScreen::BackButtonHandler(lv_event_t* e) {
  FLOG_INFO("Back button pressed nao");
  lv_obj_remove_event_cb((lv_obj_t*)lv_event_get_target(e), SettingsScreen::BackButtonHandler);
  SettingsScreen* obj = (SettingsScreen*)lv_event_get_user_data(e);
  if (!obj) return;
  if (obj->_labels->backdrop) lv_obj_delete(obj->_labels->backdrop);
  PS_PUB_NIL("ui.action.return");
  return;
  // SettingsScreen* obj = (SettingsScreen*)lv_event_get_user_data(e);
  // if (!obj) return;
  // ConfirmationContext ctx{.parent_screen = obj->GetScreen(),
  //                         .backdrop = obj->_labels->backdrop,
  //                         .object = nullptr,
  //                         .title = "Apply changes?",
  //                         .message = "Apply the new target temperature?",
  //                         .confirm_text = "Apply",
  //                         .cancel_text = "Cancel",
  //                         .on_confirm =
  //                             [obj](void*) {
  //                               FLOG_INFO("Applying new target temperature: %d°C", (int)obj->_pending_value);
  //                               // Publish new target temperature
  //                               // PS_PUB_INT("heater.target.temperature.set", obj->_pending_value);
  //                               PS_PUB_NIL("ui.action.return");

  //                               obj->_has_pending = false;
  //                               obj->_pending_value = 0;
  //                               if (obj->_labels->backdrop) lv_obj_delete(obj->_labels->backdrop);
  //                             },
  //                         .on_cancel =
  //                             [obj](void*) {
  //                               PS_PUB_NIL("ui.action.return");
  //                               FLOG_INFO("Cancelled applying new target temperature");
  //                               obj->_has_pending = false;
  //                               obj->_pending_value = 0;
  //                               if (obj->_labels->backdrop) lv_obj_delete(obj->_labels->backdrop);
  //                             }};

  // ConfirmationPopup(ctx);
}

void SettingsScreen::ResetHandler(lv_event_t* e) { esp_restart(); };

// esp_err_t SettingsScreen::GenerateFromConfig() {  // In your settings screen init:
//   ConfigEntries ui_entries = GetEntries("ui");
//   // SettingsMap ui_values;
//   GetSettings(ui_values, "ui");

//   SettingsScreen settings;
//   settings.BuildFromEntries(parent_container, "ui", ui_entries, ui_config_values);
//   return ESP_OK;
// }

// ============================================================================
// Helpers
// ============================================================================

std::vector<std::string> SettingsScreen::ParseEnumFromFormat(const std::string& format) {
  std::vector<std::string> result;
  size_t pos = format.find("enum=");
  if (pos == std::string::npos) return result;

  pos += 5;
  size_t end = format.find(',', pos);
  if (end == std::string::npos) end = format.size();

  std::string enum_part = format.substr(pos, end - pos);

  size_t start = 0;
  while (start < enum_part.size()) {
    size_t pipe = enum_part.find('|', start);
    if (pipe == std::string::npos) pipe = enum_part.size();
    result.push_back(enum_part.substr(start, pipe - start));
    start = pipe + 1;
  }
  return result;
}

Validator SettingsScreen::ParseValidatorFromFormat(const std::string& format) {
  Validator v;
  if (format.empty()) return v;

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
    }

    pos = comma + 1;
  }
  return v;
}

}  // namespace toothless