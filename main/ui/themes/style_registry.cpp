/// @file style_registry.cpp
/// @brief Global style registry implementation
#include "style_registry.hpp"

namespace toothless::themes {

// Style storage - buttons
namespace buttons {
lv_style_t primary;
lv_style_t secondary;
lv_style_t danger;
lv_style_t success;
lv_style_t settings;
lv_style_t pressed;
lv_style_t disabled;
}  // namespace buttons

// Screens and containers
namespace screens {
lv_style_t background;
lv_style_t card;
lv_style_t menu_bg;
}  // namespace screens

// Text styles
namespace text {
lv_style_t title;
lv_style_t heading;
lv_style_t body;
lv_style_t small;
lv_style_t value_large;
lv_style_t value_small;
lv_style_t unit;
}  // namespace text

// Chart styles
namespace charts {
lv_style_t background;
lv_style_t grid_lines;
lv_style_t line_temp;
lv_style_t line_target;
lv_style_t cursor;
}  // namespace charts

// Control styles
namespace controls {
lv_style_t slider_main;
lv_style_t slider_indicator;
lv_style_t slider_knob;
lv_style_t switch_bg;
lv_style_t switch_indicator;
lv_style_t switch_knob;
}  // namespace controls

// LED indicators
namespace indicators {
lv_style_t led_on;
lv_style_t led_off;
}  // namespace indicators

// Current theme state
static ColorPalette current_palette;
static ThemeId current_theme = ThemeId::REFLOW_DARK;
static bool initialized = false;

// Style initialization functions
static void init_button_styles() {
  // Primary button
  lv_style_init(&buttons::primary);
  lv_style_set_bg_color(&buttons::primary, current_palette.primary);
  lv_style_set_text_color(&buttons::primary, current_palette.on_primary);
  lv_style_set_radius(&buttons::primary, 8);
  lv_style_set_pad_all(&buttons::primary, 12);
  lv_style_set_border_width(&buttons::primary, 0);
  lv_style_set_shadow_width(&buttons::primary, 4);
  lv_style_set_shadow_color(&buttons::primary, lv_color_black());
  lv_style_set_shadow_opa(&buttons::primary, LV_OPA_30);

  // Secondary button
  lv_style_init(&buttons::secondary);
  lv_style_set_bg_color(&buttons::secondary, current_palette.secondary);
  lv_style_set_text_color(&buttons::secondary, current_palette.on_secondary);
  lv_style_set_radius(&buttons::secondary, 8);
  lv_style_set_pad_all(&buttons::secondary, 12);
  lv_style_set_border_width(&buttons::secondary, 0);

  // Danger button
  lv_style_init(&buttons::danger);
  lv_style_set_bg_color(&buttons::danger, current_palette.danger);
  lv_style_set_text_color(&buttons::danger, lv_color_white());
  lv_style_set_radius(&buttons::danger, 8);
  lv_style_set_pad_all(&buttons::danger, 12);
  lv_style_set_border_width(&buttons::danger, 2);
  lv_style_set_border_color(&buttons::danger, lv_palette_darken(LV_PALETTE_RED, 3));

  // Success button
  lv_style_init(&buttons::success);
  lv_style_set_bg_color(&buttons::success, current_palette.success);
  lv_style_set_text_color(&buttons::success, lv_color_white());
  lv_style_set_radius(&buttons::success, 8);
  lv_style_set_pad_all(&buttons::success, 12);

  // Settings button (icon button)
  lv_style_init(&buttons::settings);
  lv_style_set_bg_color(&buttons::settings, current_palette.surface);
  lv_style_set_text_color(&buttons::settings, current_palette.on_surface);
  lv_style_set_radius(&buttons::settings, 8);
  lv_style_set_pad_all(&buttons::settings, 12);
  lv_style_set_border_width(&buttons::settings, 1);
  lv_style_set_border_color(&buttons::settings, current_palette.border);

  // Pressed state (applies to all buttons)
  lv_style_init(&buttons::pressed);
  lv_style_set_bg_opa(&buttons::pressed, LV_OPA_70);
  lv_style_set_transform_scale(&buttons::pressed, 95);  // Slight shrink

  // Disabled state
  lv_style_init(&buttons::disabled);
  lv_style_set_bg_color(&buttons::disabled, current_palette.disabled);
  lv_style_set_text_color(&buttons::disabled, lv_palette_darken(LV_PALETTE_GREY, 1));
  lv_style_set_opa(&buttons::disabled, LV_OPA_50);
}

static void init_screen_styles() {
  // Screen background
  lv_style_init(&screens::background);
  lv_style_set_bg_color(&screens::background, current_palette.background);
  lv_style_set_bg_opa(&screens::background, LV_OPA_COVER);
  lv_style_set_text_color(&screens::background, current_palette.on_background);
  lv_style_set_pad_all(&screens::background, 10);

  // Card/panel surface
  lv_style_init(&screens::card);
  lv_style_set_bg_color(&screens::card, current_palette.surface);
  lv_style_set_bg_opa(&screens::card, LV_OPA_COVER);
  lv_style_set_radius(&screens::card, 12);
  lv_style_set_pad_all(&screens::card, 16);
  lv_style_set_border_width(&screens::card, 1);
  lv_style_set_border_color(&screens::card, current_palette.border);
  lv_style_set_shadow_width(&screens::card, 8);
  lv_style_set_shadow_color(&screens::card, lv_color_black());
  lv_style_set_shadow_opa(&screens::card, LV_OPA_20);

  // Menu background
  lv_style_init(&screens::menu_bg);
  lv_style_set_bg_color(&screens::menu_bg, current_palette.surface);
  lv_style_set_bg_opa(&screens::menu_bg, LV_OPA_COVER);
  lv_style_set_pad_all(&screens::menu_bg, 8);
  lv_style_set_pad_row(&screens::menu_bg, 4);
}

static void init_text_styles() {
  // Title (largest)
  lv_style_init(&text::title);
  lv_style_set_text_font(&text::title, &lv_font_montserrat_24);
  lv_style_set_text_color(&text::title, current_palette.on_background);

  // Heading
  lv_style_init(&text::heading);
  lv_style_set_text_font(&text::heading, &lv_font_montserrat_20);
  lv_style_set_text_color(&text::heading, current_palette.on_background);

  // Body text
  lv_style_init(&text::body);
  lv_style_set_text_font(&text::body, &lv_font_montserrat_16);
  lv_style_set_text_color(&text::body, current_palette.on_surface);

  // Small text
  lv_style_init(&text::small);
  lv_style_set_text_font(&text::small, &lv_font_montserrat_12);
  lv_style_set_text_color(&text::small, lv_palette_lighten(LV_PALETTE_GREY, 2));

  // Large numeric value (temperature display)
  lv_style_init(&text::value_large);
  lv_style_set_text_font(&text::value_large, &lv_font_montserrat_48);
  lv_style_set_text_color(&text::value_large, current_palette.primary);

  // Small numeric value
  lv_style_init(&text::value_small);
  lv_style_set_text_font(&text::value_small, &lv_font_montserrat_24);
  lv_style_set_text_color(&text::value_small, current_palette.on_surface);

  // Unit labels (°C, seconds, etc.)
  lv_style_init(&text::unit);
  lv_style_set_text_font(&text::unit, &lv_font_montserrat_16);
  lv_style_set_text_color(&text::unit, lv_palette_lighten(LV_PALETTE_GREY, 1));
}

static void init_chart_styles() {
  // Chart background
  lv_style_init(&charts::background);
  lv_style_set_bg_color(&charts::background, current_palette.surface);
  lv_style_set_bg_opa(&charts::background, LV_OPA_COVER);
  lv_style_set_border_width(&charts::background, 1);
  lv_style_set_border_color(&charts::background, current_palette.border);
  lv_style_set_pad_all(&charts::background, 8);

  // Grid lines
  lv_style_init(&charts::grid_lines);
  lv_style_set_line_color(&charts::grid_lines, current_palette.border);
  lv_style_set_line_width(&charts::grid_lines, 1);
  lv_style_set_line_dash_width(&charts::grid_lines, 3);
  lv_style_set_line_dash_gap(&charts::grid_lines, 3);

  // Temperature line (actual)
  lv_style_init(&charts::line_temp);
  lv_style_set_line_color(&charts::line_temp, current_palette.primary);
  lv_style_set_line_width(&charts::line_temp, 3);

  // Target temperature line
  lv_style_init(&charts::line_target);
  lv_style_set_line_color(&charts::line_target, current_palette.secondary);
  lv_style_set_line_width(&charts::line_target, 2);
  lv_style_set_line_dash_width(&charts::line_target, 5);
  lv_style_set_line_dash_gap(&charts::line_target, 5);

  // Chart cursor
  lv_style_init(&charts::cursor);
  lv_style_set_bg_color(&charts::cursor, current_palette.primary);
  lv_style_set_border_width(&charts::cursor, 2);
  lv_style_set_border_color(&charts::cursor, lv_color_white());
  lv_style_set_radius(&charts::cursor, LV_RADIUS_CIRCLE);
}

static void init_control_styles() {
  // Slider - main track
  lv_style_init(&controls::slider_main);
  lv_style_set_bg_color(&controls::slider_main, current_palette.surface);
  lv_style_set_bg_opa(&controls::slider_main, LV_OPA_COVER);
  lv_style_set_radius(&controls::slider_main, LV_RADIUS_CIRCLE);
  lv_style_set_border_width(&controls::slider_main, 1);
  lv_style_set_border_color(&controls::slider_main, current_palette.border);

  // Slider - filled indicator
  lv_style_init(&controls::slider_indicator);
  lv_style_set_bg_color(&controls::slider_indicator, current_palette.primary);
  lv_style_set_bg_opa(&controls::slider_indicator, LV_OPA_COVER);
  lv_style_set_radius(&controls::slider_indicator, LV_RADIUS_CIRCLE);

  // Slider - knob/handle
  lv_style_init(&controls::slider_knob);
  lv_style_set_bg_color(&controls::slider_knob, current_palette.primary);
  lv_style_set_bg_opa(&controls::slider_knob, LV_OPA_COVER);
  lv_style_set_radius(&controls::slider_knob, LV_RADIUS_CIRCLE);
  lv_style_set_pad_all(&controls::slider_knob, 6);
  lv_style_set_border_width(&controls::slider_knob, 2);
  lv_style_set_border_color(&controls::slider_knob, lv_color_white());

  // Switch - background
  lv_style_init(&controls::switch_bg);
  lv_style_set_bg_color(&controls::switch_bg, current_palette.disabled);
  lv_style_set_bg_opa(&controls::switch_bg, LV_OPA_COVER);
  lv_style_set_radius(&controls::switch_bg, LV_RADIUS_CIRCLE);

  // Switch - active indicator
  lv_style_init(&controls::switch_indicator);
  lv_style_set_bg_color(&controls::switch_indicator, current_palette.success);
  lv_style_set_bg_opa(&controls::switch_indicator, LV_OPA_COVER);

  // Switch - knob
  lv_style_init(&controls::switch_knob);
  lv_style_set_bg_color(&controls::switch_knob, lv_color_white());
  lv_style_set_bg_opa(&controls::switch_knob, LV_OPA_COVER);
  lv_style_set_radius(&controls::switch_knob, LV_RADIUS_CIRCLE);
  lv_style_set_pad_all(&controls::switch_knob, -4);
}

static void init_indicator_styles() {
  // LED ON state
  lv_style_init(&indicators::led_on);
  lv_style_set_bg_color(&indicators::led_on, current_palette.primary);
  lv_style_set_bg_opa(&indicators::led_on, LV_OPA_COVER);
  lv_style_set_radius(&indicators::led_on, LV_RADIUS_CIRCLE);
  lv_style_set_shadow_width(&indicators::led_on, 10);
  lv_style_set_shadow_color(&indicators::led_on, current_palette.primary);
  lv_style_set_shadow_spread(&indicators::led_on, 3);

  // LED OFF state
  lv_style_init(&indicators::led_off);
  lv_style_set_bg_color(&indicators::led_off, current_palette.disabled);
  lv_style_set_bg_opa(&indicators::led_off, LV_OPA_50);
  lv_style_set_radius(&indicators::led_off, LV_RADIUS_CIRCLE);
  lv_style_set_border_width(&indicators::led_off, 1);
  lv_style_set_border_color(&indicators::led_off, current_palette.border);
}

void Init(ThemeId theme) {
  current_theme = theme;

  // Load color palette for selected theme
  switch (theme) {
    case ThemeId::REFLOW_DARK:
      current_palette = PALETTE_REFLOW_DARK;
      break;
    case ThemeId::REFLOW_LIGHT:
      current_palette = PALETTE_REFLOW_LIGHT;
      break;
    case ThemeId::HIGH_CONTRAST:
      current_palette = PALETTE_HIGH_CONTRAST;
      break;
    case ThemeId::MINIMAL:
      current_palette = PALETTE_MINIMAL;
      break;
    default:
      current_palette = PALETTE_REFLOW_DARK;
  }

  // Initialize all style categories
  init_button_styles();
  init_screen_styles();
  init_text_styles();
  init_chart_styles();
  init_control_styles();
  init_indicator_styles();

  initialized = true;
}

void SwitchTheme(ThemeId new_theme) {
  if (!initialized) {
    Init(new_theme);
    return;
  }

  // Reset all styles before re-initializing
  lv_style_reset(&buttons::primary);
  lv_style_reset(&buttons::secondary);
  lv_style_reset(&buttons::danger);
  lv_style_reset(&buttons::success);
  lv_style_reset(&buttons::settings);
  lv_style_reset(&buttons::pressed);
  lv_style_reset(&buttons::disabled);

  lv_style_reset(&screens::background);
  lv_style_reset(&screens::card);
  lv_style_reset(&screens::menu_bg);

  lv_style_reset(&text::title);
  lv_style_reset(&text::heading);
  lv_style_reset(&text::body);
  lv_style_reset(&text::small);
  lv_style_reset(&text::value_large);
  lv_style_reset(&text::value_small);
  lv_style_reset(&text::unit);

  lv_style_reset(&charts::background);
  lv_style_reset(&charts::grid_lines);
  lv_style_reset(&charts::line_temp);
  lv_style_reset(&charts::line_target);
  lv_style_reset(&charts::cursor);

  lv_style_reset(&controls::slider_main);
  lv_style_reset(&controls::slider_indicator);
  lv_style_reset(&controls::slider_knob);
  lv_style_reset(&controls::switch_bg);
  lv_style_reset(&controls::switch_indicator);
  lv_style_reset(&controls::switch_knob);

  lv_style_reset(&indicators::led_on);
  lv_style_reset(&indicators::led_off);

  // Re-initialize with new theme
  Init(new_theme);

  // Trigger style refresh on all existing objects
  lv_obj_report_style_change(NULL);
}

const ColorPalette& GetColors() { return current_palette; }

ThemeId GetCurrentTheme() { return current_theme; }

}  // namespace toothless::themes
