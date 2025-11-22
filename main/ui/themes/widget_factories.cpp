/// @file widget_factories.cpp
/// @brief Pre-styled widget factory implementations
#include "widget_factories.hpp"

#include "style_registry.hpp"

namespace toothless::ui {

// ============================================================================
// BUTTON FACTORIES
// ============================================================================

lv_obj_t* CreatePrimaryButton(lv_obj_t* parent, const char* text, int32_t width, int32_t height, bool grow) {
  lv_obj_t* btn = lv_button_create(parent);
  lv_obj_add_style(btn, &themes::buttons::primary, 0);
  lv_obj_add_style(btn, &themes::buttons::pressed, LV_STATE_PRESSED);
  lv_obj_add_style(btn, &themes::buttons::disabled, LV_STATE_DISABLED);

  lv_obj_set_size(btn, width, height);
  lv_obj_set_flex_grow(btn, grow);

  lv_obj_t* label = lv_label_create(btn);
  lv_label_set_text(label, text);
  lv_obj_center(label);

  return btn;
}

lv_obj_t* CreatePrimaryButton(lv_obj_t* parent, const char* text, bool grow) {
  return CreatePrimaryButton(parent, text, LV_SIZE_CONTENT, LV_SIZE_CONTENT, grow);
}

lv_obj_t* CreateSecondaryButton(lv_obj_t* parent, const char* text, bool grow) {
  lv_obj_t* btn = lv_button_create(parent);
  lv_obj_add_style(btn, &themes::buttons::secondary, 0);
  lv_obj_add_style(btn, &themes::buttons::pressed, LV_STATE_PRESSED);
  lv_obj_add_style(btn, &themes::buttons::disabled, LV_STATE_DISABLED);

  if (grow) {
    lv_obj_set_size(btn, LV_SIZE_CONTENT, lv_pct(100));
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
    lv_obj_set_size(btn, LV_SIZE_CONTENT, lv_pct(100));
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
    lv_obj_set_size(btn, LV_SIZE_CONTENT, lv_pct(100));
    lv_obj_set_flex_grow(btn, 1);
  } else {
    lv_obj_set_size(btn, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
  }

  lv_obj_t* label = lv_label_create(btn);
  lv_label_set_text(label, text);
  lv_obj_center(label);

  return btn;
}

lv_obj_t* CreateSettingsButton(lv_obj_t* parent, bool grow) {
  lv_obj_t* btn = lv_button_create(parent);
  lv_obj_add_style(btn, &themes::buttons::settings, 0);
  lv_obj_add_style(btn, &themes::buttons::pressed, LV_STATE_PRESSED);

  if (grow) {
    lv_obj_set_size(btn, LV_SIZE_CONTENT, lv_pct(100));
    // lv_obj_set_flex_grow(btn, 1);
  } else {
    lv_obj_set_size(btn, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
  }

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
  lv_obj_remove_flag(screen, LV_OBJ_FLAG_SCROLLABLE);
  return screen;
}

lv_obj_t* CreateSubScreen(lv_obj_t* parent) {
  lv_obj_t* screen = lv_obj_create(parent);
  lv_obj_remove_flag(screen, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_remove_flag(screen, LV_OBJ_FLAG_FLEX_IN_NEW_TRACK);
  lv_obj_add_flag(screen, LV_OBJ_FLAG_FLOATING);
  lv_obj_add_style(screen, &themes::screens::subscreen, 0);
  lv_obj_set_size(screen, lv_pct(100), lv_pct(100));
  lv_obj_set_pos(screen, 0, 0);

  return screen;
}

lv_obj_t* CreateCard(lv_obj_t* parent) {
  lv_obj_t* card = lv_obj_create(parent);
  lv_obj_add_style(card, &themes::screens::card, 0);
  lv_obj_remove_flag(card, LV_OBJ_FLAG_SCROLLABLE);

  return card;
}

lv_obj_t* CreateRowContainer(lv_obj_t* parent) {
  lv_obj_t* container = lv_obj_create(parent);
  lv_obj_add_style(container, &themes::screens::rowcontainer, 0);
  lv_obj_remove_flag(container, LV_OBJ_FLAG_SCROLLABLE);
  return container;
}

lv_obj_t* CreateColumnContainer(lv_obj_t* parent) {
  lv_obj_t* container = lv_obj_create(parent);
  lv_obj_add_style(container, &themes::screens::columncontainer, 0);
  lv_obj_remove_flag(container, LV_OBJ_FLAG_SCROLLABLE);
  return container;
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

lv_obj_t* CreateRoller(lv_obj_t* parent, const char* options, int32_t selected) {
  lv_obj_t* roller = lv_roller_create(parent);
  lv_obj_add_style(roller, &themes::controls::roller, 0);
  lv_obj_add_style(roller, &themes::controls::roller_selected, LV_PART_SELECTED);
  lv_roller_set_visible_row_count(roller, 4);

  lv_roller_set_options(roller, options, LV_ROLLER_MODE_NORMAL);
  lv_roller_set_selected(roller, selected, LV_ANIM_OFF);
  lv_obj_center(roller);

  return roller;
}

// ============================================================================
// CHART FACTORIES
// ============================================================================

lv_obj_t* CreateChart(lv_obj_t* parent, uint16_t points_capacity) {
  lv_obj_t* chart = lv_chart_create(parent);
  lv_obj_add_style(chart, &themes::charts::background, 0);

  lv_chart_set_type(chart, LV_CHART_TYPE_LINE);
  lv_chart_set_point_count(chart, points_capacity);
  lv_chart_set_update_mode(chart, LV_CHART_UPDATE_MODE_SHIFT);
  lv_chart_set_div_line_count(chart, 5, 10);

  lv_obj_add_style(chart, &themes::charts::line_temp, LV_PART_ITEMS);
  lv_obj_add_style(chart, &themes::charts::indicator, LV_PART_INDICATOR);
  lv_obj_add_style(chart, &themes::charts::cursor, LV_PART_CURSOR);

  return chart;
}

// ============================================================================
// INDICATOR FACTORIES
// ============================================================================

lv_obj_t* CreateLEDIndicator(lv_obj_t* parent, bool initial_state) {
  lv_obj_t* led = lv_led_create(parent);

  // need to set color with lv_led_color_set(), the theme doesn't actually change the color.
  lv_led_set_brightness(led, LV_LED_BRIGHT_MAX);
  lv_led_set_color(led, themes::GetColors().danger);

  if (initial_state) {
    lv_obj_add_style(led, &themes::indicators::led_on, 0);
    lv_led_on(led);
  } else {
    lv_obj_add_style(led, &themes::indicators::led_off, 0);
    lv_led_off(led);
  }

  return led;
}

lv_obj_t* CreateMessageBox(lv_obj_t* parent, std::string title, std::string message, std::string confirm_text,
                           std::string cancel_text, std::function<void(void*)> on_confirm,
                           std::function<void(void*)> on_cancel, void* user_data) {
  lv_obj_t* msgbox = lv_msgbox_create(parent);
  lv_obj_add_style(msgbox, &themes::menus::messagebox, 0);
  lv_obj_add_style(msgbox, &themes::menus::messagebox_backdrop, LV_PART_MAIN | LV_STATE_DEFAULT);

  lv_obj_set_size(msgbox, lv_pct(90), lv_pct(90));
  // lv_obj_set_flex_align(msgbox, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

  lv_obj_move_to_index(msgbox, -1);
  lv_obj_set_style_text_align(msgbox, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);

  if (!message.empty()) lv_msgbox_add_text(msgbox, message.c_str());
  if (!title.empty()) lv_msgbox_add_title(msgbox, title.c_str());

  // if (cancel_text != "") {
  //   lv_obj_t* cancel_button = lv_msgbox_add_footer_button(msgbox, cancel_text.c_str());
  //   lv_obj_set_width(cancel_button, lv_pct(45));
  //   lv_obj_set_height(cancel_button, 100);
  //   // lv_obj_set_flex_grow(cancel_button, 1); // Chart grows to fill remaining space
  //   if (on_cancel) lv_obj_add_event_cb(cancel_button, on_cancel, LV_EVENT_CLICKED, user_data);
  // }

  // if (confirm_text != "") {
  //   lv_obj_t* confirm_button = lv_msgbox_add_footer_button(msgbox, confirm_text.c_str());
  //   lv_obj_set_width(confirm_button, lv_pct(40));
  //   lv_obj_set_height(confirm_button, 100);
  //   // lv_obj_set_flex_grow(confirm_button, 1); // Chart grows to fill remaining space
  //   if (on_confirm) lv_obj_add_event_cb(confirm_button, on_confirm, LV_EVENT_CLICKED, user_data);
  // }
  // lv_obj_t* footer = lv_msgbox_get_footer(msgbox);
  // lv_obj_set_height(footer, lv_pct(33));
  return msgbox;
}

// ============================================================================
// HELPER FUNCTIONS
// ============================================================================

void UpdateValueLabel(lv_obj_t* label, float value, const char* format) {
  // BUG: this doesn't work, for some reason the label text doesn't update properly
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
