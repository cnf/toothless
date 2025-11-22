/// @file widget_factories.cpp
/// @brief Pre-styled widget factory implementations
#include "widget_factories.hpp"

#include "style_registry.hpp"

namespace toothless::ui {

// ============================================================================
// BUTTON FACTORIES
// ============================================================================

lv_obj_t* CreatePrimaryButton(lv_obj_t* parent, const char* text, bool grow) {
  lv_obj_t* btn = lv_button_create(parent);
  lv_obj_add_style(btn, &themes::buttons::primary, 0);
  lv_obj_add_style(btn, &themes::buttons::pressed, LV_STATE_PRESSED);
  lv_obj_add_style(btn, &themes::buttons::disabled, LV_STATE_DISABLED);

  if (grow) {
    lv_obj_set_size(btn, lv_pct(100), LV_SIZE_CONTENT);
    lv_obj_set_flex_grow(btn, 1);
  } else {
    lv_obj_set_size(btn, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
  }

  lv_obj_t* label = lv_label_create(btn);
  lv_label_set_text(label, text);
  lv_obj_center(label);

  return btn;
}

lv_obj_t* CreateSecondaryButton(lv_obj_t* parent, const char* text, bool grow) {
  lv_obj_t* btn = lv_button_create(parent);
  lv_obj_add_style(btn, &themes::buttons::secondary, 0);
  lv_obj_add_style(btn, &themes::buttons::pressed, LV_STATE_PRESSED);
  lv_obj_add_style(btn, &themes::buttons::disabled, LV_STATE_DISABLED);

  if (grow) {
    lv_obj_set_size(btn, lv_pct(100), LV_SIZE_CONTENT);
    lv_obj_set_flex_grow(btn, 1);
  } else {
    lv_obj_set_size(btn, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
  }

  lv_obj_t* label = lv_label_create(btn);
  lv_label_set_text(label, text);
  lv_obj_center(label);

  return btn;
}

lv_obj_t* CreateDangerButton(lv_obj_t* parent, const char* text, bool grow) {
  lv_obj_t* btn = lv_button_create(parent);
  lv_obj_add_style(btn, &themes::buttons::danger, 0);
  lv_obj_add_style(btn, &themes::buttons::pressed, LV_STATE_PRESSED);
  lv_obj_add_style(btn, &themes::buttons::disabled, LV_STATE_DISABLED);

  if (grow) {
    lv_obj_set_size(btn, lv_pct(100), LV_SIZE_CONTENT);
    lv_obj_set_flex_grow(btn, 1);
  } else {
    lv_obj_set_size(btn, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
  }

  lv_obj_t* label = lv_label_create(btn);
  lv_label_set_text(label, text);
  lv_obj_center(label);

  return btn;
}

lv_obj_t* CreateSuccessButton(lv_obj_t* parent, const char* text, bool grow) {
  lv_obj_t* btn = lv_button_create(parent);
  lv_obj_add_style(btn, &themes::buttons::success, 0);
  lv_obj_add_style(btn, &themes::buttons::pressed, LV_STATE_PRESSED);
  lv_obj_add_style(btn, &themes::buttons::disabled, LV_STATE_DISABLED);

  if (grow) {
    lv_obj_set_size(btn, lv_pct(100), LV_SIZE_CONTENT);
    lv_obj_set_flex_grow(btn, 1);
  } else {
    lv_obj_set_size(btn, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
  }

  lv_obj_t* label = lv_label_create(btn);
  lv_label_set_text(label, text);
  lv_obj_center(label);

  return btn;
}

lv_obj_t* CreateSettingsButton(lv_obj_t* parent) {
  lv_obj_t* btn = lv_button_create(parent);
  lv_obj_add_style(btn, &themes::buttons::settings, 0);
  lv_obj_add_style(btn, &themes::buttons::pressed, LV_STATE_PRESSED);

  lv_obj_set_size(btn, LV_SIZE_CONTENT, LV_SIZE_CONTENT);

  lv_obj_t* label = lv_label_create(btn);
  lv_label_set_text(label, LV_SYMBOL_SETTINGS);
  lv_obj_center(label);

  return btn;
}

// ============================================================================
// SCREEN & CONTAINER FACTORIES
// ============================================================================

lv_obj_t* CreateScreen() {
  lv_obj_t* screen = lv_obj_create(NULL);
  lv_obj_add_style(screen, &themes::screens::background, 0);
  lv_obj_clear_flag(screen, LV_OBJ_FLAG_SCROLLABLE);
  return screen;
}

lv_obj_t* CreateCard(lv_obj_t* parent) {
  lv_obj_t* card = lv_obj_create(parent);
  lv_obj_add_style(card, &themes::screens::card, 0);
  return card;
}

lv_obj_t* CreateMenuContainer(lv_obj_t* parent) {
  lv_obj_t* menu = lv_obj_create(parent);
  lv_obj_add_style(menu, &themes::screens::menu_bg, 0);
  lv_obj_set_flex_flow(menu, LV_FLEX_FLOW_COLUMN);
  return menu;
}

// ============================================================================
// TEXT/LABEL FACTORIES
// ============================================================================

lv_obj_t* CreateTitle(lv_obj_t* parent, const char* text) {
  lv_obj_t* label = lv_label_create(parent);
  lv_label_set_text(label, text);
  lv_obj_add_style(label, &themes::text::title, 0);
  return label;
}

lv_obj_t* CreateHeading(lv_obj_t* parent, const char* text) {
  lv_obj_t* label = lv_label_create(parent);
  lv_label_set_text(label, text);
  lv_obj_add_style(label, &themes::text::heading, 0);
  return label;
}

lv_obj_t* CreateBodyText(lv_obj_t* parent, const char* text) {
  lv_obj_t* label = lv_label_create(parent);
  lv_label_set_text(label, text);
  lv_obj_add_style(label, &themes::text::body, 0);
  return label;
}

lv_obj_t* CreateSmallText(lv_obj_t* parent, const char* text) {
  lv_obj_t* label = lv_label_create(parent);
  lv_label_set_text(label, text);
  lv_obj_add_style(label, &themes::text::small, 0);
  return label;
}

lv_obj_t* CreateValueLarge(lv_obj_t* parent, float value, const char* format) {
  lv_obj_t* label = lv_label_create(parent);
  lv_obj_add_style(label, &themes::text::value_large, 0);
  UpdateValueLabel(label, value, format);
  return label;
}

lv_obj_t* CreateValueSmall(lv_obj_t* parent, int value, const char* format) {
  lv_obj_t* label = lv_label_create(parent);
  lv_obj_add_style(label, &themes::text::value_small, 0);
  lv_label_set_text_fmt(label, format, value);
  return label;
}

lv_obj_t* CreateUnitLabel(lv_obj_t* parent, const char* unit) {
  lv_obj_t* label = lv_label_create(parent);
  lv_label_set_text(label, unit);
  lv_obj_add_style(label, &themes::text::unit, 0);
  return label;
}

// ============================================================================
// CONTROL FACTORIES
// ============================================================================

lv_obj_t* CreateSlider(lv_obj_t* parent, int32_t min, int32_t max, int32_t value) {
  lv_obj_t* slider = lv_slider_create(parent);
  lv_obj_add_style(slider, &themes::controls::slider_main, LV_PART_MAIN);
  lv_obj_add_style(slider, &themes::controls::slider_indicator, LV_PART_INDICATOR);
  lv_obj_add_style(slider, &themes::controls::slider_knob, LV_PART_KNOB);

  lv_slider_set_range(slider, min, max);
  lv_slider_set_value(slider, value, LV_ANIM_OFF);

  return slider;
}

lv_obj_t* CreateSwitch(lv_obj_t* parent, bool initial_state) {
  lv_obj_t* sw = lv_switch_create(parent);
  lv_obj_add_style(sw, &themes::controls::switch_bg, LV_PART_MAIN);
  lv_obj_add_style(sw, &themes::controls::switch_indicator, LV_PART_INDICATOR | LV_STATE_CHECKED);
  lv_obj_add_style(sw, &themes::controls::switch_knob, LV_PART_KNOB);

  if (initial_state) {
    lv_obj_add_state(sw, LV_STATE_CHECKED);
  }

  return sw;
}

// ============================================================================
// CHART FACTORIES
// ============================================================================

lv_obj_t* CreateTemperatureChart(lv_obj_t* parent, uint16_t points_capacity) {
  lv_obj_t* chart = lv_chart_create(parent);
  lv_obj_add_style(chart, &themes::charts::background, 0);

  lv_chart_set_type(chart, LV_CHART_TYPE_LINE);
  lv_chart_set_point_count(chart, points_capacity);
  lv_chart_set_update_mode(chart, LV_CHART_UPDATE_MODE_SHIFT);

  // Create temperature series
  lv_chart_series_t* ser_temp = lv_chart_add_series(chart, themes::GetColors().primary, LV_CHART_AXIS_PRIMARY_Y);
  lv_obj_add_style(chart, &themes::charts::line_temp, LV_PART_ITEMS);

  // Create target temperature series
  lv_chart_series_t* ser_target = lv_chart_add_series(chart, themes::GetColors().secondary, LV_CHART_AXIS_PRIMARY_Y);
  // Note: Cannot apply different styles to different series easily in LVGL
  // You'd need to handle this manually or use multiple chart objects

  // Style the cursor
  lv_obj_add_style(chart, &themes::charts::cursor, LV_PART_CURSOR);

  return chart;
}

// ============================================================================
// INDICATOR FACTORIES
// ============================================================================

lv_obj_t* CreateLEDIndicator(lv_obj_t* parent, bool initial_state) {
  lv_obj_t* led = lv_led_create(parent);

  if (initial_state) {
    lv_obj_add_style(led, &themes::indicators::led_on, 0);
    lv_led_on(led);
  } else {
    lv_obj_add_style(led, &themes::indicators::led_off, 0);
    lv_led_off(led);
  }

  return led;
}

// ============================================================================
// HELPER FUNCTIONS
// ============================================================================

void UpdateValueLabel(lv_obj_t* label, float value, const char* format) {
  if (label == NULL) return;
  lv_label_set_text_fmt(label, format, value);
}

void SetLEDState(lv_obj_t* led, bool on) {
  if (led == NULL) return;

  if (on) {
    lv_obj_remove_style(led, &themes::indicators::led_off, 0);
    lv_obj_add_style(led, &themes::indicators::led_on, 0);
    lv_led_on(led);
  } else {
    lv_obj_remove_style(led, &themes::indicators::led_on, 0);
    lv_obj_add_style(led, &themes::indicators::led_off, 0);
    lv_led_off(led);
  }
}

}  // namespace toothless::ui
