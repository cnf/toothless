#include "ui/screens/running_screen.hpp"

#include "funlog.h"
#include "ui/display/display.hpp"
#include <algorithm>
// #include <cmath>
#include <esp_timer.h>

extern "C" {
#include <pubsub.h>
}

namespace toothless {

RunningScreen::RunningScreen() { _labels = std::make_unique<RunningScreenLabels>(); }

RunningScreen::~RunningScreen() {
  if (_update_timer) {
    lv_timer_set_repeat_count(_update_timer, 0); // Stop repeating
    lv_timer_delete(_update_timer);
    _update_timer = nullptr;
  }
  if (_subscription) {
    ps_free_subscriber(_subscription);
  }
}

lv_obj_t *RunningScreen::Create() {
  _target_temp = 35; // TODO: make configurable
  _subscription = ps_new_subscriber(10, PS_STRLIST("sensor.chamber.temperature"));
  _screen = lv_obj_create(NULL);
  lv_obj_set_style_pad_all(_screen, 10, 0); // Global 2% border

  // Set screen to vertical flex layout
  lv_obj_set_layout(_screen, LV_LAYOUT_FLEX);
  lv_obj_set_flex_flow(_screen, LV_FLEX_FLOW_COLUMN); // Vertical stacking
  lv_obj_set_style_pad_gap(_screen, 10, 0);           // 10px gap between items

  Temperature();
  Chart();
  MidSection();
  BottomRow();
  _update_timer = lv_timer_create(UIUpdateTimerCB, kUIUpdateIntervalMs, this);
  return _screen;
}

void RunningScreen::Loop() {}

void RunningScreen::UIUpdateTimerCB(lv_timer_t *timer) {
  RunningScreen *screen = (RunningScreen *)lv_timer_get_user_data(timer);
  if (screen) {
    screen->UpdateAllDisplays();
  }
}

esp_err_t RunningScreen::UpdateAllDisplays() {
  ps_msg_t *msg = nullptr;
  for ((msg = ps_get(_subscription, 0)); msg != NULL; (msg = ps_get(_subscription, 0))) {
    if (ps_has_topic(msg, "sensor.chamber.temperature") && PS_IS_INT(msg)) {
      // FLOG_DEBUG("Received temperature: %d", (int)msg->int_val);
      TemperatureUpdateCurrent((uint32_t)msg->int_val);
    } else if (ps_has_topic(msg, "heater.target.temperature") && PS_IS_INT(msg)) {
      TemperatureUpdateTarget(msg->int_val);
      FLOG_INFO("TODO");
    }
    ps_unref_msg(msg);
  }
  return ESP_OK;
}

esp_err_t RunningScreen::Chart() {
  lv_obj_t *wrapper = lv_obj_create(_screen);
  lv_obj_remove_style_all(wrapper);
  lv_obj_set_size(wrapper, lv_pct(100), lv_pct(35));
  lv_obj_set_layout(wrapper, LV_LAYOUT_FLEX);
  lv_obj_set_flex_flow(wrapper, LV_FLEX_FLOW_ROW);
  // lv_obj_set_style_align(wrapper, LV_ALIGN_LEFT_MID, 0);

  // Chart
  _labels->chart = lv_chart_create(wrapper);
  if (!_labels->chart) {
    FLOG_ERROR("Failed to create chart");
    return ESP_ERR_NO_MEM;
  }
  lv_obj_set_size(_labels->chart, 0, lv_pct(100));
  lv_obj_set_flex_grow(_labels->chart, 1); // Chart grows to fill remaining space
  lv_chart_set_update_mode(_labels->chart, LV_CHART_UPDATE_MODE_SHIFT);
  _labels->chart_series = lv_chart_add_series(_labels->chart, lv_palette_main(LV_PALETTE_RED), LV_CHART_AXIS_PRIMARY_Y);
  lv_chart_set_type(_labels->chart, LV_CHART_TYPE_LINE);
  // Do not display points on the data
  lv_obj_set_style_size(_labels->chart, 0, 0, LV_PART_INDICATOR);

  // // Add grid styling to chart
  // lv_obj_set_style_bg_color(_labels->chart, lv_palette_darken(LV_PALETTE_GREY, 5), 0);
  // lv_obj_set_style_border_width(_labels->chart, 1, 0);
  // lv_obj_set_style_border_color(_labels->chart, lv_palette_main(LV_PALETTE_GREY), 0);

  // Scale
  _labels->chart_scale_right = lv_scale_create(wrapper);
  lv_scale_set_mode(_labels->chart_scale_right, LV_SCALE_MODE_VERTICAL_RIGHT);
  lv_obj_set_size(_labels->chart_scale_right, 25, lv_pct(100));
  lv_obj_set_flex_grow(_labels->chart_scale_right, 0); // Don't grow
  // lv_obj_set_style_pad_top(_labels->chart_scale_right, 10, 0);
  // lv_obj_set_style_pad_bottom(_labels->chart_scale_right, 10, 0);
  lv_scale_set_total_tick_count(_labels->chart_scale_right, Y_LABEL_COUNT);
  lv_scale_set_major_tick_every(_labels->chart_scale_right, 1);
  // TODO: see of this needs dynamic calc for different screen
  // lv_obj_set_style_pad_ver(_labels->chart_scale_right, lv_chart_get_first_point_center_offset(_labels->chart), 0);
  lv_obj_set_style_pad_ver(_labels->chart_scale_right, 10, 0); // Fixed 10px padding
  // lv_scale_set_text_src(_labels->chart_scale_right, month);
  lv_obj_set_style_text_font(_labels->chart_scale_right, &lv_font_montserrat_10, 0);

  ChartSetScale();
  lv_chart_set_point_count(_labels->chart, 100); // Keep last 100 points

  return ESP_OK;
}

esp_err_t RunningScreen::ChartSetScale() { return ChartSetScale(_target_temp); }

esp_err_t RunningScreen::ChartSetScale(int32_t scale) {
  // TODO: Detect maximum value on the chart atm, we don't want to be smaller than that.
  // Scale to show target + 20% margin
  int32_t margin = scale * 0.2f;
  int32_t chart_max = scale + margin;
  // Round up to nearest 50 for clean scale
  if (scale > 100) {
    chart_max = ((chart_max + 49) / 50) * 50;
  } else {
    chart_max = ((chart_max + 9) / 10) * 10;
  }
  lv_chart_set_axis_range(_labels->chart, LV_CHART_AXIS_PRIMARY_Y, 0, chart_max);
  ChartYAxisLabels(0, chart_max);
  lv_obj_invalidate(_labels->chart); // Force redraw
  FLOG_INFO("Set chart to scale: %li", chart_max);
  return ESP_OK;
};

void RunningScreen::ChartYAxisLabels(int32_t min_temp, int32_t max_temp) {
  int32_t temp_range = max_temp - min_temp;
  for (int i = 0; i < Y_LABEL_COUNT; i++) {
    int32_t temp_value = min_temp + (temp_range * i) / (Y_LABEL_COUNT - 1);

    // char temp_str[16];
    // snprintf(temp_str, sizeof(temp_str), "%li°", temp_value);
    // y_left_labels[i] = temp_str;
    snprintf(label_strings[i], sizeof(label_strings[i]), "%li°", temp_value);
    label_pointers[i] = label_strings[i]; // Point to the string

    // Position relative to chart (0,0 is chart's top-left)
    // int32_t label_y = chart_height - (chart_height * i) / (label_count - 1);
  }
  // y_left_labels[-1] = NULL;
  label_pointers[Y_LABEL_COUNT] = nullptr; // NULL terminate

  lv_scale_set_text_src(_labels->chart_scale_right, label_pointers);
}

int32_t RunningScreen::ChartGetMaxValue() {
  if (!_labels->chart_series)
    return 0;

  // Get the actual number of points currently displayed
  uint16_t point_count = lv_chart_get_point_count(_labels->chart);
  int32_t max_value = 0;

  // Only look at currently visible/stored points
  int32_t *series = lv_chart_get_series_y_array(_labels->chart, _labels->chart_series);
  for (uint16_t i = 0; i < point_count; i++) {
    int32_t value = series[i];
    // lv_coord_t value = lv_chart_get_point_value_by_id(_labels->chart, _labels->chart_series, i);
    // Skip "empty" points (LVGL might use LV_CHART_POINT_NONE or negative values for empty)
    if (value != LV_CHART_POINT_NONE && value > max_value) {
      max_value = value;
    }
  }
  return max_value;
}

esp_err_t RunningScreen::Temperature() {
  std::lock_guard<std::mutex> lock(Display::GetLvglMutex());
  lv_obj_t *temp_container = lv_obj_create(_screen);
  lv_obj_set_style_pad_all(temp_container, 0, 0); // Remove all padding
  lv_obj_set_scrollbar_mode(temp_container, LV_SCROLLBAR_MODE_OFF);
  lv_obj_clear_flag(temp_container, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_size(temp_container, lv_pct(100), lv_pct(10));
  lv_obj_set_layout(temp_container, LV_LAYOUT_FLEX);
  lv_obj_set_flex_flow(temp_container, LV_FLEX_FLOW_ROW); // Side by side
  lv_obj_set_style_pad_gap(temp_container, 10, 0);        // Gap between temp blocks

  _labels->temp_current = TemperatureBlock(temp_container, "Current", "--°C");
  _labels->temp_target = TemperatureBlock(temp_container, "Target", "--°C");

  return ESP_OK;
}

lv_obj_t *RunningScreen::TemperatureBlock(lv_obj_t *parent, const char *title, const char *temp) {
  lv_obj_t *obj;
  lv_obj_t *temperature_obj = lv_obj_create(parent);
  lv_obj_set_style_pad_all(temperature_obj, 0, 0); // Remove all padding

  lv_obj_set_flex_grow(temperature_obj, 1); // Equal width temperature_objs
  lv_obj_set_layout(temperature_obj, LV_LAYOUT_FLEX);
  lv_obj_set_flex_flow(temperature_obj, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_style_pad_gap(temperature_obj, 2, 0); // ← ADD THIS LINE - 2px gap instead of default
  lv_obj_set_flex_align(temperature_obj, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

  // Title
  lv_obj_t *cur_title_label = lv_label_create(temperature_obj);
  lv_label_set_text(cur_title_label, title);
  lv_obj_set_style_text_font(cur_title_label, &lv_font_montserrat_10, 0);
  // lv_obj_set_size(cur_title_label, lv_pct(10), LV_SIZE_CONTENT); // Auto height

  // Temperature
  obj = lv_label_create(temperature_obj);
  lv_label_set_text(obj, temp);
  lv_obj_set_style_text_font(obj, &lv_font_montserrat_22, 0);
  // lv_obj_set_size(_labels->temp_current, lv_pct(90), LV_SIZE_CONTENT); // Auto height
  return obj;
};

void RunningScreen::TemperatureUpdateCurrent(int32_t temp) {
  FLOG_DEBUG("Received temperature: %d", temp);
  float temperature = temp / 100.0f;
  char temp_str[16];
  static uint32_t last_scale_update = 0;
  uint32_t now = esp_timer_get_time() / 1000; // milliseconds

  // float clamped_temp = fminf(fmaxf(temperature, -999.99f), 9999.99f);
  float clamped_temp = std::clamp(temperature, -999.99f, 9999.99f);
  snprintf(temp_str, sizeof(temp_str), "%.2f°C", clamped_temp);
  // No mutex needed - we're already in LVGL task!
  if (_labels->temp_current) {
    lv_label_set_text(_labels->temp_current, temp_str);
  }
  if (_labels->chart) {
    lv_chart_set_next_value(_labels->chart, _labels->chart_series, temperature);

    // Only update scale every 1 second
    if (now - last_scale_update > 1000) {
      int32_t max = ChartGetMaxValue();
      if (max >= _target_temp) {
        ChartSetScale(max);
        last_scale_update = now;
      }
    }
  }
}

void RunningScreen::TemperatureUpdateTarget(int32_t temp) {
  FLOG_DEBUG("Received target: %d", temp);
  char temp_str[16];
  int32_t clamped_temp = std::clamp<int32_t>(temp, int32_t(-999), int32_t(9999));
  snprintf(temp_str, sizeof(temp_str), "%li°C", clamped_temp);
  // No mutex needed - we're already in LVGL task!
  lv_label_set_text(_labels->temp_target, temp_str);
}

esp_err_t RunningScreen::MidSection() {
  // After Chart() and before Button():
  lv_obj_t *spacer = lv_obj_create(_screen);
  lv_obj_remove_style_all(spacer); // Make it invisible
  lv_obj_set_style_bg_opa(spacer, LV_OPA_TRANSP, 0);
  lv_obj_set_flex_grow(spacer, 1); // This spacer grows to fill remaining space
  lv_obj_set_height(spacer, 0);    // Minimum height

  return ESP_OK;
}

esp_err_t RunningScreen::TemperatureSlider() {
  std::lock_guard<std::mutex> lock(Display::GetLvglMutex());
  lv_obj_t *temp_slider = lv_slider_create(_screen);
  lv_obj_set_size(temp_slider, 200, 40);
  lv_obj_align(temp_slider, LV_ALIGN_TOP_MID, 100, 80);

  lv_obj_add_event_cb(temp_slider, TemperatureSliderHandler, LV_EVENT_VALUE_CHANGED, this);

  return ESP_OK;
}

void RunningScreen::TemperatureSliderHandler(lv_event_t *e) {
  // TODO: only update when you let go of the slider
  lv_obj_t *slider = (lv_obj_t *)lv_event_get_target(e);
  int32_t value = lv_slider_get_value(slider);
  FLOG_INFO("Temperature set to: %d", value);
}

esp_err_t RunningScreen::BottomRow() {
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
  StopButton(temp_container);
  return ESP_OK;
}

/// @brief StopButton to start reflow
/// @return
esp_err_t RunningScreen::StopButton(lv_obj_t *container) {
  // Create a button

  lv_obj_t *stop_btn = lv_button_create(container);
  lv_obj_set_size(stop_btn, lv_pct(50), lv_pct(100));
  // lv_obj_set_flex_grow(stop_btn, 1); // grows to fill remaining space

  // lv_obj_align(stop_btn, LV_ALIGN_BOTTOM_RIGHT, lv_pct(2), lv_pct(-2));

  lv_obj_t *stop_label = lv_label_create(stop_btn);
  lv_label_set_text(stop_label, "Stop");
  lv_obj_center(stop_label);
  lv_obj_set_style_align(stop_btn, LV_ALIGN_BOTTOM_RIGHT, 0); // or LV_ALIGN_RIGHT

  // Register event with screen object as user data
  lv_obj_add_event_cb(stop_btn, StopButtonEventHandler, LV_EVENT_CLICKED, this);

  return ESP_OK;
}

void RunningScreen::StopButtonEventHandler(lv_event_t *e) {
  // FLOG_INFO("StopButton handler");
  lv_event_code_t code = lv_event_get_code(e);
  // lv_obj_t *btn = (lv_obj_t *)lv_event_get_target(e);

  if (code == LV_EVENT_CLICKED) {

    // Get the screen object if you passed it as user_data
    RunningScreen *screen = (RunningScreen *)lv_event_get_user_data(e);
    if (screen) {
      screen->StopButtonPress();
    }
  }
}

void RunningScreen::StopButtonPress() {
  FLOG_INFO("Stop StopButton clicked!");
  // PS_PUB_NIL("ui.action.stop");
  StopConfirmation();
  // Switch to running screen
  // userInterface->SwitchTo(ScreenState::RUNNING);
}

void RunningScreen::StopConfirmation() {
  // Create a backdrop to block background interactions
  lv_obj_t *backdrop = lv_obj_create(_screen);
  lv_obj_set_size(backdrop, lv_pct(100), lv_pct(100));
  lv_obj_set_pos(backdrop, 0, 0);
  lv_obj_clear_flag(backdrop, LV_OBJ_FLAG_FLEX_IN_NEW_TRACK);
  lv_obj_add_flag(backdrop, LV_OBJ_FLAG_FLOATING);

  // Style the backdrop - semi-transparent dark overlay
  lv_obj_set_style_bg_color(backdrop, lv_color_black(), 0);
  lv_obj_set_style_bg_opa(backdrop, 128, 0); // 50% opacity
  lv_obj_set_style_border_width(backdrop, 0, 0);
  lv_obj_remove_style(backdrop, NULL, LV_PART_SCROLLBAR);
  lv_obj_clear_flag(backdrop, LV_OBJ_FLAG_SCROLLABLE);

  lv_obj_t *msgbox = lv_msgbox_create(backdrop);
  lv_obj_set_size(msgbox, lv_pct(75), lv_pct(50));

  // Remove from flex layout so it can be positioned freely
  lv_obj_clear_flag(msgbox, LV_OBJ_FLAG_FLEX_IN_NEW_TRACK);
  lv_obj_add_flag(msgbox, LV_OBJ_FLAG_FLOATING); // Make it float

  lv_obj_center(msgbox);
  // lv_obj_add_flag(msgbox, LV_OBJ_FLAG_FLOATING);
  lv_obj_move_to_index(msgbox, -1); // Move to top of children

  const char *title = {"Stops"};
  const char *text = {"Are you sure you want to stop the reflow process?"};
  lv_msgbox_add_text(msgbox, text);
  lv_msgbox_add_title(msgbox, title);

  // lv_msgbox_add_close_button(msgbox);

  lv_obj_t *cancel_button = lv_msgbox_add_footer_button(msgbox, "Cancel");
  lv_obj_add_event_cb(cancel_button, ButtonCB, LV_EVENT_CLICKED, backdrop);

  lv_obj_t *stop_button = lv_msgbox_add_footer_button(msgbox, "Stop");
  lv_obj_add_event_cb(stop_button, ButtonCB, LV_EVENT_CLICKED, backdrop);
}

void RunningScreen::ButtonCB(lv_event_t *e) {
  lv_obj_t *button = (lv_obj_t *)lv_event_get_target(e);
  lv_obj_t *backdrop = (lv_obj_t *)lv_event_get_user_data(e);

  // Get button text to determine action
  lv_obj_t *label = lv_obj_get_child(button, 0); // Button's label
  const char *text = lv_label_get_text(label);

  if (strcmp(text, "Stop") == 0) {
    FLOG_INFO("Stop confirmed!");
    PS_PUB_NIL("ui.action.stop");
    // lv_async_call([](void *) { PS_PUB_NIL("ui.action.stop"); }, nullptr);
    return;
  } else if (strcmp(text, "Cancel") == 0) {
    FLOG_INFO("Cancelled");
    if (backdrop) {
      lv_obj_delete_async(backdrop);
    }
  }
}

} // namespace toothless
