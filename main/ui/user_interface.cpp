#include "user_interface.hpp"

#include <driver/gpio.h>
#include <esp_check.h>
#include <esp_err.h>
#include <lvgl.h>

#include "funlog.h"
#include "helpers.hpp"
#include "ui/chart_history.hpp"
#include "ui/display/display.hpp"
#include "ui/screens/dryer_screen.hpp"
#include "ui/screens/reflow_screen.hpp"
#include "ui/screens/settings_screen.hpp"

namespace toothless {

UserInterface::UserInterface() {
  _display = nullptr;
  _current_screen_state = ScreenList::kDryerScreen;
  _current_screen_obj = nullptr;  // Initialize before calling SwitchTo
  _current_screen = nullptr;
  _switching_screen_state = false;

  // TODO: make topic strings configurations
  // _chart_history = ChartHistory();
  _chart_history.New("sensor.temperature.chamber");
  _chart_history.New("heater.target.temperature", true);
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
}

esp_err_t UserInterface::Start() {
  ESP_RETURN_ON_ERROR(Display::Init(), FLOG_SHORT_FILENAME, "Display Initialization failed");

  lv_async_call(
      [](void*) {
        UserInterface* ui = new UserInterface();
        ui->Init();
      },
      nullptr);
  return ESP_OK;
}

esp_err_t UserInterface::Init() {
  esp_log_level_set(FLOG_SHORT_FILENAME, ESP_LOG_DEBUG);
  _subscription = ps_new_subscriber(10, PS_STRLIST("ui.action", "heater.mode"));

  // ESP_RETURN_ON_ERROR(Display::Init(), FLOG_SHORT_FILENAME, "Display Initialization failed");

  _display = Display::GetDisplayPtr();
  FLOG_DEBUG("Display ptr: %p, lv_display_get_default: %p, free heap: %u", _display, lv_display_get_default(),
             esp_get_free_heap_size());

  // lv_timer_t* ui_timer = lv_timer_create([this](void*) { this->Loop(); }, 100, this);
  // lv_timer_t* t =
  lv_timer_create(
      +[](lv_timer_t* timer) {
        UserInterface* ui = static_cast<UserInterface*>(lv_timer_get_user_data(timer));
        ui->Loop();
      },
      50, this);

  // lv_timer_t* ui_timer =
  lv_timer_create(
      +[](lv_timer_t* timer) {
        UserInterface* ui = static_cast<UserInterface*>(lv_timer_get_user_data(timer));
        ui->SwitchTo(ui->_current_screen_state);
        lv_timer_del(timer);  // or lv_timer_set_repeat_count(timer, 1);
      },
      200 /*ms*/, this);

  // SwitchTo(_current_screen_state);

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
  ESP_ERROR_CHECK(HandleSubscriptions());
  if (_current_screen) {
    _current_screen->Loop();
  }
  return;
}

esp_err_t UserInterface::SwitchTo(ScreenList screen) {
  // return ESP_OK;
  FLOG_INFO("Switching Screens: %d", screen);
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

    // Now safely replace the old with new
    _current_screen = std::move(new_screen);  // Old screen auto-destructs here
    _current_screen_state = screen;
    if (_current_screen_obj) {
      lv_obj_delete(_current_screen_obj);  //<! Delete old screen object
      FLOG_DEBUG("Deleted old screen object: %p", _current_screen_obj);
    }
    _current_screen_obj = new_screen_obj;
  }
  FLOG_DEBUG("Completed screen switch");

  _switching_screen_state = false;
  return ESP_OK;
}

esp_err_t UserInterface::HandleSubscriptions() {
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
        default:
          SwitchTo(ScreenList::kDryerScreen);
          break;
      }
    } else if (ps_has_topic(msg, "ui.action.stop")) {
      SwitchTo(ScreenList::kDryerScreen);
    } else if (ps_has_topic(msg, "ui.action.settings")) {
      SwitchTo(ScreenList::kSettingsScreen);
    } else if (ps_has_topic(msg, "heater.mode.set")) {
    } else if (ps_has_topic(msg, "heater.mode") && PS_IS_INT(msg)) {
      heater::Mode new_mode = static_cast<heater::Mode>(msg->int_val);
      // if (new_mode != _mode) {
      if (true) {
        FLOG_INFO("Heater mode changed to %d", static_cast<int>(new_mode));
        _mode = new_mode;
        // switch (_current_screen_state) {
        //   // TODO: expand modes/states
        //   case ScreenList::kErrorScreen:
        //   case ScreenList::kSettingsScreen:
        //     FLOG_DEBUG("Current ScreenState: ERROR or SETTINGS");
        //     ps_unref_msg(msg);
        //     continue;
        //     break;
        // }
        switch (_mode) {
          case heater::Mode::kModeReflow:
            FLOG_DEBUG("REFLOW mode");
            SwitchTo(ScreenList::kReflowScreen);
            break;
          case heater::Mode::kModeDrying:
            FLOG_DEBUG("DRYING mode");
            SwitchTo(ScreenList::kDryerScreen);
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
      // switch (_current_screen_state) {
      //   // TODO: expand modes/states
      //   case ScreenList::kErrorScreen:
      //     FLOG_DEBUG("Current ScreenState: ERROR");
      //     break;
      //   case ScreenList::kSettingsScreen:
      //     FLOG_DEBUG("Current ScreenState: SETTINGS");
      //     break;
      //   case ScreenList::kReflowScreen:
      //     FLOG_DEBUG("Current ScreenState: REFLOW");
      //     if (new_mode == heater::Mode::kModeDrying || new_mode == heater::Mode::kModeCooldown) {
      //       SwitchTo(ScreenList::kDryerScreen);
      //     }
      //     break;
      //   case ScreenList::kDryerScreen:
      //     FLOG_DEBUG("Current ScreenState: DRYER");
      //     if (new_mode == heater::Mode::kModeHeating || new_mode == heater::Mode::kModeReflow) {
      //       SwitchTo(ScreenList::kReflowScreen);
      //     }
      //     break;
      //   default:
      //     FLOG_ERROR("Unhandled screen state: %d", static_cast<int>(_current_screen_state));
      //     break;
      // }
    } else {
      FLOG_ERROR("Unhandled topic: %s", msg->topic);
    }
    ps_unref_msg(msg);
  }
  return ESP_OK;
}

}  // namespace toothless