#include "user_interface.hpp"

#include <driver/gpio.h>
#include <esp_check.h>
#include <esp_err.h>
#include <lvgl.h>

#include "funlog.h"
#include "heater/heater.hpp"
#include "ui/chart_history.hpp"
#include "ui/display/display.hpp"
#include "ui/screens/autotune_screen.hpp"
#include "ui/screens/dryer_screen.hpp"
#include "ui/screens/profiles_screen.hpp"
#include "ui/screens/reflow_screen.hpp"
#include "ui/screens/settings_screen.hpp"
#include "ui/subjects.hpp"
#include "ui/themes/style_registry.hpp"

namespace toothless {

UserInterface::UserInterface() {
  _display = nullptr;
  // _current_screen_state = ScreenList::kDryerScreen;
  _current_screen_obj = nullptr;  // Initialize before calling SwitchTo
  _current_screen = nullptr;
  _switching_screen_state = false;
  _config = std::make_shared<SettingsMap>();
  _subjects = SubjectManager::Instance().subjects;

  // TODO: make topic strings configurations
  // _chart_history = ChartHistory();
  _chart_history.New("sensor.temperature.zone");
  _chart_history.New(topics::heater::target_temperature, true);
}

UserInterface::~UserInterface() {
  // TODO: figure out what all needs unique/shared ptrs
  if (_current_screen_obj) {
    lv_obj_delete_async(_current_screen_obj);
    _current_screen_obj = nullptr;
  }
  if (_subscription) {
    ps_free_subscriber(_subscription);
    _subscription = nullptr;
  }
  if (_config_entries) {
    delete _config_entries;
    _config_entries = nullptr;
  }
  FLOG_INFO("UserInterface destructed");
}

esp_err_t UserInterface::Start() {
  lv_async_call(
      [](void*) {
        // FIXME:: memory leak? how do we clean this up on shutdown?
        UserInterface* ui = new UserInterface();
        vTaskDelay(pdMS_TO_TICKS(1000));  // give the rest of the code time to settle
        ui->Init();
      },
      nullptr);
  return ESP_OK;
}

esp_err_t UserInterface::Init() {
  // esp_log_level_set(FLOG_SHORT_FILENAME, ESP_LOG_DEBUG);
  _config_entries = new ConfigEntries;
  _config_entries->insert(std::end(*_config_entries), std::begin(ui_config_entries), std::end(ui_config_entries));
  RegisterConfig(_config_entries, topics::ui::name);
  FLOG_DEBUG("Waiting for settings...");
  GetSettings(_config, topics::ui::name);

  SubjectManager::Instance().Init();

  _subscription = ps_new_subscriber(10, PS_STRLIST(topics::ui::name, topics::heater::mode, kTopicStatus));

  _display = Display::GetDisplayPtr();
  FLOG_DEBUG("Display ptr: %p, lv_display_get_default: %p, free heap: %u", _display, lv_display_get_default(),
             esp_get_free_heap_size());

  ApplySettings();

  lv_timer_create(
      +[](lv_timer_t* timer) {
        UserInterface* ui = static_cast<UserInterface*>(lv_timer_get_user_data(timer));
        ui->Loop();
      },
      50, this);

#if defined(CONFIG_LV_USE_SYSMON)
  /* Create generic monitor */
  _sysmon = lv_sysmon_create(lv_display_get_default());
#if defined(CONFIG_LV_USE_PERF_MONITOR)
  /* Create performance monitor */
  lv_sysmon_show_performance(NULL); /* NULL = default display */
#endif
#if defined(CONFIG_LV_USE_MEM_MONITOR)
  /* Create memory monitor */
  lv_sysmon_show_memory(NULL);
#endif
#endif
  return ESP_OK;
}

void UserInterface::Loop() {
  // value member — safe
  _chart_history.Loop();
  SubjectManager::Instance().Loop();
  ESP_ERROR_CHECK(HandleSubscriptions());
  if (_current_screen) {
    _current_screen->Loop();
  }
  return;
}

esp_err_t UserInterface::SwitchTo(ScreenList screen) {
  // return ESP_OK;
  FLOG_DEBUG("Switching Screens: %d", screen);
  // If we're already switching, ignore the request
  if (_switching_screen_state) {
    return ESP_OK;
  }
  // stops you from switching OUT of
  // switch (_current_screen_state) {
  //   case ScreenList::kErrorScreen:
  //   case ScreenList::kSettingsScreen:
  //     FLOG_DEBUG("Current ScreenState: ERROR or SETTINGS");
  //     return ESP_OK;
  //     break;
  //   default:
  //     break;
  // }
  // Store the target state and defer the actual switch
  _switching_screen_state = true;
  std::unique_ptr<Screen> new_screen;
  FLOG_DEBUG("Requested ScreenState: %d", screen);
  switch (screen) {
    case ScreenList::kDryerScreen:
      new_screen = std::make_unique<DryerScreen>();
      FLOG_DEBUG("Switching to Dryer Screen");
      break;
    case ScreenList::kReflowScreen:
      new_screen = std::make_unique<ReflowScreen>(&_chart_history);
      FLOG_DEBUG("Switching to Reflow Screen");
      break;
    case ScreenList::kSettingsScreen:
      new_screen = std::make_unique<SettingsScreen>();
      FLOG_DEBUG("Switching to SETTINGS Screen");
      break;
    case ScreenList::kProfilesScreen:
      FLOG_DEBUG("Switching to PROFILES Screen");
      new_screen = std::make_unique<ProfilesScreen>();
      break;
    case ScreenList::kAutotuneScreen:
      FLOG_DEBUG("Switching to AUTOTUNE Screen");
      new_screen = std::make_unique<AutotuneScreen>(&_chart_history);
      break;
    default:
      FLOG_ERROR("ScreenState %d not implemented", screen);
      _switching_screen_state = false;  // ← Also reset flag
      return ESP_ERR_NOT_SUPPORTED;
  };

  if (new_screen) {
    FLOG_DEBUG("Created new screen instance");
    lv_obj_t* new_screen_obj = new_screen->Create();
    if (new_screen_obj == nullptr) {
      FLOG_ERROR("new_screen_obj is NULL");
      _switching_screen_state = false;
      return ESP_ERR_NO_MEM;
    }
    if (lv_display_get_default() == nullptr) {
      FLOG_ERROR("No LVGL default display");
      _switching_screen_state = false;
      return ESP_ERR_INVALID_STATE;
    }

    lv_screen_load(new_screen_obj);  //<! Switch to new screen (LVGL handles old screen cleanup)
    FLOG_DEBUG("SwitchTo: new_screen_obj=%p free_heap=%u", new_screen_obj, esp_get_free_heap_size());
    // lv_screen_load_anim(new_screen_obj, LV_SCREEN_LOAD_ANIM_FADE_IN, 250, 0, true);
    // lv_refr_now(NULL);  //<! force screen refresh, so the new screen loads faster

    if (_current_screen_obj) {
      lv_obj_delete(_current_screen_obj);  // Delete LVGL objects FIRST
      _current_screen_obj = nullptr;
      FLOG_DEBUG("Deleted old screen object: %p", _current_screen_obj);
    }
    _current_screen = std::move(new_screen);  // THEN destroy C++ objects
    _current_screen_obj = new_screen_obj;
  }
  FLOG_DEBUG("Completed screen switch");

  _switching_screen_state = false;
  return ESP_OK;
}

esp_err_t UserInterface::HandleSubscriptions() {
  bool new_settings = false;
  size_t len = 0;
  ps_msg_t* msg = nullptr;
  for ((msg = ps_get(_subscription, 0)); msg != NULL; (msg = ps_get(_subscription, 0))) {
    FLOG_DEBUG("MSG TOPIC: %s", msg->topic);
    if (ps_has_topic(msg, "ui.action.return")) {
      switch (_mode) {
        case heater::Mode::kModeHeating:
        case heater::Mode::kModeReflow:
          SwitchTo(ScreenList::kReflowScreen);
          break;
        case heater::Mode::kModeCooldown:
        case heater::Mode::kModeDrying:
          SwitchTo(ScreenList::kDryerScreen);
          break;
        case heater::Mode::kModeTune:
          SwitchTo(ScreenList::kAutotuneScreen);
          break;
        default:
          SwitchTo(ScreenList::kDryerScreen);
          break;
      }
    } else if (ps_has_topic(msg, "ui.action.stop")) {
      SwitchTo(ScreenList::kDryerScreen);
    } else if (ps_has_topic(msg, "ui.action.settings")) {
      SwitchTo(ScreenList::kSettingsScreen);
    } else if (ps_has_topic(msg, "ui.action.profiles")) {
      SwitchTo(ScreenList::kProfilesScreen);
    } else if (ps_has_topic(msg, topics::heater::mode_set)) {
    } else if (ps_has_topic(msg, topics::heater::mode) && PS_IS_INT(msg)) {
      heater::Mode new_mode = static_cast<heater::Mode>(msg->int_val);
      if (true) {
        FLOG_DEBUG("Heater mode changed to %d", static_cast<int>(new_mode));
        _mode = new_mode;
        switch (_mode) {
          case heater::Mode::kModeReflow:
            FLOG_DEBUG("REFLOW mode");
            SwitchTo(ScreenList::kReflowScreen);
            break;
          case heater::Mode::kModeDrying:
            FLOG_DEBUG("DRYING mode");
            SwitchTo(ScreenList::kDryerScreen);
            break;
          case heater::Mode::kModeTune:
            FLOG_DEBUG("TUNE mode");
            SwitchTo(ScreenList::kAutotuneScreen);
            break;
          case heater::Mode::kModeHeating:
          case heater::Mode::kModeCooldown:
            FLOG_ERROR("Heater is in HEATING or COOLDOWN mode, no screen switch");
            break;
          default:
            FLOG_DEBUG("Heater is in UNKNOWN mode");
            break;
        }
      }
    } else if (ps_has_topic_suffix(msg, kTopicConfigGet) && PS_IS_NIL(msg)) {
      FLOG_DEBUG("Sending config map");
      GetSettings(_config, topics::ui::name);
      new_settings = true;
      len++;
    } else if (ps_has_topic_suffix(msg, kTopicConfigSet) && PS_IS_NIL(msg)) {
      FLOG_DEBUG("Applying settings");
      new_settings = true;
      len++;
    } else if (ps_has_topic(msg, kTopicStatus)) {
      if (PS_IS_ERR(msg)) {
        ps_err_t err_msg = msg->err_val;
        // std::string status_text = std::string(err_msg.desc);
        FLOG_ERROR("%s: %s", esp_err_to_name(err_msg.id), err_msg.desc);
        CreateWarning(std::string(esp_err_to_name(err_msg.id)).c_str(), err_msg.desc);
      } else if (PS_IS_STR(msg)) {
        // FLOG_ERROR("Status: %s", msg->str_val);
        CreateWarning("Warning", msg->str_val);
      } else {
        FLOG_ERROR("Error receiving status message: %d", msg->err_val);
      }
    } else {
      FLOG_ERROR("Unhandled topic: %s", msg->topic);
    }
    ps_unref_msg(msg);
  }
  if (new_settings) {
    ApplySettings();
  }
  return ESP_OK;
}

esp_err_t UserInterface::ApplySettings() {
  FLOG_INFO("Applying UI settings");
  themes::SwitchTheme(themes::FromString(std::get<std::string>(_config->at("theme"))));
  lv_subject_set_int(&_subjects->sidebar, std::get<bool>(_config->at("sidebar")) ? 1 : 0);
  Display::SetBrightness(static_cast<uint8_t>(std::get<int>(_config->at("brightness"))));

  return ESP_OK;
}

}  // namespace toothless