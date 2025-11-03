#include "ui/screens/home_screen.hpp"

#include "funlog.h"
#include "ui/display/display.hpp"
extern "C" {
#include <pubsub.h>
}

namespace toothless {

HomeScreen::HomeScreen() { _labels = std::make_unique<HomeScreenLabels>(); }

HomeScreen::~HomeScreen() {
  if (_update_timer) {
    lv_timer_delete(_update_timer);
  }
  if (_subscription) {
    ps_free_subscriber(_subscription);
  }
}

lv_obj_t *HomeScreen::Create() {
  _subscription = ps_new_subscriber(10, PS_STRLIST("sensor.chamber.temperature"));
  _screen = lv_obj_create(NULL);
  lv_obj_set_style_pad_all(_screen, 10, 0); // Global 2% border
                                            // Set screen to vertical flex layout
  lv_obj_set_layout(_screen, LV_LAYOUT_FLEX);
  lv_obj_set_flex_flow(_screen, LV_FLEX_FLOW_COLUMN); // Vertical stacking
  lv_obj_set_style_pad_gap(_screen, 10, 0);           // 10px gap between items

  Temperature();
  MidSection();
  BottomRow();

  _update_timer = lv_timer_create(UIUpdateTimerCB, kUIUpdateIntervalMs, this); // Update every 100ms
  return _screen;
}
void HomeScreen::Loop() {}

void HomeScreen::UIUpdateTimerCB(lv_timer_t *timer) {
  HomeScreen *screen = (HomeScreen *)lv_timer_get_user_data(timer);
  if (screen) {
    screen->UpdateAllDisplays();
  }
}

esp_err_t HomeScreen::UpdateAllDisplays() {
  ps_msg_t *msg = nullptr;
  for ((msg = ps_get(_subscription, 0)); msg != NULL; (msg = ps_get(_subscription, 0))) {
    if (ps_has_topic(msg, "sensor.chamber.temperature") && PS_IS_INT(msg)) {
      // FLOG_DEBUG("Received temperature: %d", (int)msg->int_val);
      UpdateTemperatureDisplay((uint32_t)msg->int_val);
    } else if (ps_has_topic(msg, "sensor.somethingelse") && PS_IS_INT(msg)) {
      FLOG_INFO("TODO");
    }
    ps_unref_msg(msg);
  }
  return ESP_OK;
}

esp_err_t HomeScreen::Temperature() {
  std::lock_guard<std::mutex> lock(Display::GetLvglMutex());
  lv_obj_t *temp_container = lv_obj_create(_screen);
  lv_obj_set_style_pad_all(temp_container, 0, 0); // Remove all padding
  lv_obj_set_scrollbar_mode(temp_container, LV_SCROLLBAR_MODE_OFF);
  lv_obj_clear_flag(temp_container, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_size(temp_container, lv_pct(100), lv_pct(10));
  lv_obj_set_layout(temp_container, LV_LAYOUT_FLEX);
  lv_obj_set_flex_flow(temp_container, LV_FLEX_FLOW_ROW); // Side by side
  lv_obj_set_style_pad_gap(temp_container, 10, 0);        // Gap between temp blocks

  // _labels->temp_current = TemperatureBlock(temp_container, "Current", "--°C");
  lv_obj_t *temperature_obj = lv_obj_create(temp_container);
  lv_obj_set_style_pad_all(temperature_obj, 0, 0); // Remove all padding

  lv_obj_set_flex_grow(temperature_obj, 1); // Equal width temperature_objs
  lv_obj_set_layout(temperature_obj, LV_LAYOUT_FLEX);
  lv_obj_set_flex_flow(temperature_obj, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_style_pad_gap(temperature_obj, 2, 0); // ← ADD THIS LINE - 2px gap instead of default
  lv_obj_set_flex_align(temperature_obj, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

  // // Title
  // lv_obj_t *cur_title_label = lv_label_create(temperature_obj);
  // lv_label_set_text(cur_title_label, title);
  // lv_obj_set_style_text_font(cur_title_label, &lv_font_montserrat_10, 0);
  // // lv_obj_set_size(cur_title_label, lv_pct(10), LV_SIZE_CONTENT); // Auto height

  // Temperature
  _labels->temperature = lv_label_create(temperature_obj);
  lv_label_set_text(_labels->temperature, "----");
  lv_obj_set_style_text_font(_labels->temperature, &lv_font_montserrat_22, 0);

  return ESP_OK;
}

void HomeScreen::UpdateTemperatureDisplay(uint32_t temp) {
  FLOG_DEBUG("Received temperature: %d", temp);
  // float temperature = temp / 100.0f;
  static uint32_t last_temp = UINT32_MAX;
  static char temp_str[16];

  if (temp != last_temp) {
    // std::lock_guard<std::mutex> lock(Display::GetLvglMutex());
    float temperature = temp / 100.0f;
    snprintf(temp_str, sizeof(temp_str), "%.2f°C", temperature);
    lv_label_set_text(_labels->temperature, temp_str);
    last_temp = temp;
  }
  // No mutex needed - we're already in LVGL task!
  // lv_label_set_text(_labels->temperature, temp_str);
}

esp_err_t HomeScreen::MidSection() {
  // After Chart() and before Button():
  lv_obj_t *spacer = lv_obj_create(_screen);
  lv_obj_remove_style_all(spacer); // Make it invisible
  lv_obj_set_style_bg_opa(spacer, LV_OPA_TRANSP, 0);
  lv_obj_set_flex_grow(spacer, 1); // This spacer grows to fill remaining space
  lv_obj_set_height(spacer, 0);    // Minimum height
  lv_obj_t *ohai = lv_label_create(spacer);
  lv_label_set_text(ohai, "Toothless Reflow");

  return ESP_OK;
}

esp_err_t HomeScreen::BottomRow() {
  lv_obj_t *temp_container;
  {
    // std::lock_guard<std::mutex> lock(Display::GetLvglMutex());
    temp_container = lv_obj_create(_screen);
    lv_obj_remove_style_all(temp_container); // Make it invisible
    lv_obj_set_style_bg_opa(temp_container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_pad_all(temp_container, 0, 0); // Remove all padding
    lv_obj_set_scrollbar_mode(temp_container, LV_SCROLLBAR_MODE_OFF);
    lv_obj_clear_flag(temp_container, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_size(temp_container, lv_pct(100), lv_pct(15));
    lv_obj_set_layout(temp_container, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(temp_container, LV_FLEX_FLOW_ROW); // Side by side
    lv_obj_set_style_pad_gap(temp_container, 10, 0);        // Gap between temp blocks
    // lv_obj_set_flex_align(temp_container, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_END);
  }
  StartButton(temp_container);
  return ESP_OK;
}

/// @brief Button to start reflow
/// @return
esp_err_t HomeScreen::StartButton(lv_obj_t *container) {
  /////////

  lv_obj_t *start_btn = lv_button_create(container);
  lv_obj_set_size(start_btn, lv_pct(50), lv_pct(100));
  // lv_obj_align(start_btn, LV_ALIGN_BOTTOM_LEFT, lv_pct(2), lv_pct(-2));

  lv_obj_t *start_label = lv_label_create(start_btn);
  lv_label_set_text(start_label, "Start");
  // lv_obj_center(start_label);
  lv_obj_set_flex_grow(start_btn, 1); // Equal width temperature_objs

  // Register event with screen object as user data
  lv_obj_add_event_cb(start_btn, StartButtonEventHandler, LV_EVENT_CLICKED, this);
  return ESP_OK;
}

void HomeScreen::StartButtonEventHandler(lv_event_t *e) {
  FLOG_INFO("Button handler");
  lv_event_code_t code = lv_event_get_code(e);

  // lv_obj_t *btn = (lv_obj_t *)lv_event_get_target(e);

  if (code == LV_EVENT_CLICKED) {
    FLOG_INFO("Button clicked!");

    // Get the screen object if you passed it as user_data
    HomeScreen *screen = (HomeScreen *)lv_event_get_user_data(e);
    if (screen) {
      screen->HandleStartButtonPress();
    }
  }
}

void HomeScreen::HandleStartButtonPress() {
  FLOG_INFO("Handling button press in HomeScreen");
  PS_PUB_NIL("ui.action.start");
  // Switch to running screen
  // userInterface->SwitchTo(ScreenState::RUNNING);
}

} // namespace toothless
