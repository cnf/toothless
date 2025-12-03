/// @file style_registry.cpp
/// @brief Global style registry implementation
#include "style_registry.hpp"

#include "funlog.h"

LV_FONT_DECLARE(AdwaitaMonoB_128);
LV_FONT_DECLARE(AdwaitaMonoB_96);
LV_FONT_DECLARE(AdwaitaMonoB_64);
LV_FONT_DECLARE(AdwaitaMonoB_48);
LV_FONT_DECLARE(AdwaitaMonoB_32);
LV_FONT_DECLARE(AdwaitaMonoB_28);
LV_FONT_DECLARE(AdwaitaMonoB_21);

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
lv_style_t subscreen;
lv_style_t card;
lv_style_t container;
lv_style_t rowcontainer;
lv_style_t columncontainer;
lv_style_t menu;
lv_style_t menu_header;
lv_style_t menu_page;
lv_style_t menu_container;
lv_style_t menu_section;
lv_style_t menu_selected;
lv_style_t menu_unselected;
lv_style_t menu_bg;
lv_style_t sidebar;
lv_style_t sidebar_bg;
lv_style_t sidebar_button;
lv_style_t sidebar_button_active;
lv_style_t sidebar_button_inactive;
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
lv_style_t textentry;
lv_style_t danger;
lv_style_t warning;
lv_style_t success;
}  // namespace text

// Chart styles
namespace charts {
lv_style_t background;
lv_style_t grid;
lv_style_t line_temp;
lv_style_t line_target;
lv_style_t cursor;
lv_style_t indicator;
lv_style_t scale;
}  // namespace charts

// Control styles
namespace controls {
lv_style_t slider_main;
lv_style_t slider_indicator;
lv_style_t slider_knob;
lv_style_t switch_bg;
lv_style_t switch_indicator;
lv_style_t switch_knob;
lv_style_t roller;
lv_style_t roller_selected;
lv_style_t dropdown;
lv_style_t dropdown_selected;
lv_style_t dropdown_button;
}  // namespace controls

// LED indicators
namespace indicators {
lv_style_t led_on;
lv_style_t led_off;
}  // namespace indicators

namespace menus {
lv_style_t messagebox;
lv_style_t messagebox_backdrop;
lv_style_t messagebox_title;
lv_style_t messagebox_message;
lv_style_t messagebox_button;
lv_style_t msgbox_backdrop;

}  // namespace menus

namespace fonts {
lv_font_t tiny;
lv_font_t small;
lv_font_t medium;
lv_font_t large;
lv_font_t xlarge;
lv_font_t numbers_small;
lv_font_t numbers_medium;
lv_font_t numbers_large;
}  // namespace fonts

namespace debug {
lv_style_t debug;
}  // namespace debug

// Current theme state
static ColorPalette current_palette;
static Settings current_settings;
static ThemeId current_theme = ThemeId::TOOTHLESS;
static bool initialized = false;

// Style initialization functions
static void init_button_styles() {
  // Primary button
  lv_style_init(&buttons::primary);
  lv_style_set_bg_color(&buttons::primary, current_palette.primary);
  lv_style_set_text_color(&buttons::primary, current_palette.on_primary);
  lv_style_set_radius(&buttons::primary, 8);
  lv_style_set_height(&buttons::primary, current_settings.button_height);
  // lv_style_set_pad_all(&buttons::primary, 12);
  lv_style_set_text_font(&buttons::primary, &fonts::medium);
  // if (current) lv_style_set_border_width(&buttons::primary, 0);
  lv_style_set_shadow_width(&buttons::primary, 4);
  lv_style_set_shadow_color(&buttons::primary, lv_color_black());
  lv_style_set_shadow_opa(&buttons::primary, LV_OPA_30);

  // Secondary button
  lv_style_init(&buttons::secondary);
  lv_style_set_bg_color(&buttons::secondary, current_palette.secondary);
  lv_style_set_text_color(&buttons::secondary, current_palette.on_secondary);
  lv_style_set_radius(&buttons::secondary, 8);
  lv_style_set_height(&buttons::secondary, current_settings.button_height);

  // lv_style_set_pad_all(&buttons::secondary, 12);
  lv_style_set_border_width(&buttons::secondary, 0);
  lv_style_set_text_font(&buttons::secondary, &fonts::medium);
  // lv_style_set_width(&buttons::secondary, LV_SIZE_CONTENT);

  // Danger button
  lv_style_init(&buttons::danger);
  lv_style_set_bg_color(&buttons::danger, current_palette.danger);
  lv_style_set_text_color(&buttons::danger, lv_color_white());
  lv_style_set_radius(&buttons::danger, 8);
  lv_style_set_height(&buttons::danger, current_settings.button_height);

  // lv_style_set_pad_all(&buttons::danger, 12);
  lv_style_set_border_width(&buttons::danger, 2);
  lv_style_set_border_color(&buttons::danger, lv_palette_darken(LV_PALETTE_RED, 3));
  lv_style_set_text_font(&buttons::danger, &fonts::medium);
  // lv_style_set_width(&buttons::danger, LV_SIZE_CONTENT);

  // Success button
  lv_style_init(&buttons::success);
  lv_style_set_bg_color(&buttons::success, current_palette.success);
  lv_style_set_text_color(&buttons::success, lv_color_white());
  lv_style_set_radius(&buttons::success, 8);
  lv_style_set_height(&buttons::success, current_settings.button_height);
  // lv_style_set_pad_all(&buttons::success, 12);
  lv_style_set_text_font(&buttons::success, &fonts::medium);

  // Settings button (icon button)
  lv_style_init(&buttons::settings);
  lv_style_set_bg_color(&buttons::settings, current_palette.primary);
  lv_style_set_text_color(&buttons::settings, current_palette.on_primary);
  lv_style_set_radius(&buttons::settings, 8);
  lv_style_set_height(&buttons::settings, current_settings.button_height);

  // lv_style_set_pad_all(&buttons::settings, 12);
  // lv_style_set_border_width(&buttons::settings, 10);
  // lv_style_set_border_color(&buttons::settings, current_palette.border);
  lv_style_set_shadow_width(&buttons::primary, 4);
  lv_style_set_shadow_color(&buttons::primary, lv_color_black());
  lv_style_set_shadow_opa(&buttons::primary, LV_OPA_30);

  // Pressed state (applies to all buttons)
  lv_style_init(&buttons::pressed);
  lv_style_set_bg_opa(&buttons::pressed, LV_OPA_70);
  lv_style_set_radius(&buttons::settings, 16);
  // lv_style_set_transform_scale(&buttons::pressed, 240);  // Slight shrink

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
  lv_style_set_pad_all(&screens::background, current_settings.screen_padding);
  lv_style_set_layout(&screens::background, LV_LAYOUT_FLEX);
  lv_style_set_text_font(&screens::background, &fonts::medium);

  // Sub-screen (transparent overlay)
  lv_style_init(&screens::subscreen);
  lv_style_set_bg_color(&screens::subscreen, current_palette.background);
  lv_style_set_bg_opa(&screens::subscreen, LV_OPA_COVER);
  lv_style_set_text_color(&screens::subscreen, current_palette.on_background);
  lv_style_set_border_width(&screens::subscreen, 0);
  lv_style_set_radius(&screens::subscreen, 0);
  lv_style_set_pad_all(&screens::subscreen, 0);
  lv_style_set_layout(&screens::subscreen, LV_LAYOUT_FLEX);
  lv_style_set_flex_flow(&screens::subscreen, LV_FLEX_FLOW_COLUMN);
  lv_style_set_flex_main_place(&screens::subscreen, LV_FLEX_ALIGN_CENTER);
  lv_style_set_flex_cross_place(&screens::subscreen, LV_FLEX_ALIGN_CENTER);
  lv_style_set_flex_track_place(&screens::subscreen, LV_FLEX_ALIGN_CENTER);
  lv_style_set_text_font(&screens::subscreen, &fonts::medium);

  // Card/panel surface
  lv_style_init(&screens::card);
  lv_style_set_bg_color(&screens::card, lv_color_lighten(current_palette.surface, LV_OPA_10));
  lv_style_set_bg_opa(&screens::card, LV_OPA_COVER);
  lv_style_set_radius(&screens::card, 12);
  lv_style_set_border_color(&screens::card, current_palette.border);
  lv_style_set_pad_all(&screens::card, current_settings.element_padding);
  if (current_settings.borders) {
    lv_style_set_border_width(&screens::card, LV_DPX(8));
  } else {
    lv_style_set_border_width(&screens::card, 0);
  }
  lv_style_set_shadow_color(&screens::card, lv_color_black());
  lv_style_set_shadow_opa(&screens::card, LV_OPA_20);
  lv_style_set_layout(&screens::card, LV_LAYOUT_FLEX);
  lv_style_set_flex_flow(&screens::card, LV_FLEX_FLOW_ROW);
  lv_style_set_flex_main_place(&screens::card, LV_FLEX_ALIGN_START);
  lv_style_set_flex_cross_place(&screens::card, LV_FLEX_ALIGN_START);
  lv_style_set_flex_track_place(&screens::card, LV_FLEX_ALIGN_START);
  lv_style_set_text_font(&screens::card, &fonts::medium);
  // lv_style_set_pad_gap(&screens::card, LV_DPX(5));

  // Container
  lv_style_init(&screens::container);
  lv_style_set_layout(&screens::container, LV_LAYOUT_FLEX);
  lv_style_set_bg_opa(&screens::container, LV_OPA_TRANSP);
  lv_style_set_size(&screens::container, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
  lv_style_set_pad_all(&screens::container, current_settings.element_padding);
  lv_style_set_border_width(&screens::container, 0);

  // Row container
  lv_style_init(&screens::rowcontainer);
  lv_style_set_layout(&screens::rowcontainer, LV_LAYOUT_FLEX);
  lv_style_set_bg_opa(&screens::rowcontainer, LV_OPA_TRANSP);
  lv_style_set_flex_flow(&screens::rowcontainer, LV_FLEX_FLOW_ROW);
  lv_style_set_flex_main_place(&screens::rowcontainer, LV_FLEX_ALIGN_START);
  lv_style_set_flex_cross_place(&screens::rowcontainer, LV_FLEX_ALIGN_CENTER);
  lv_style_set_flex_track_place(&screens::rowcontainer, LV_FLEX_ALIGN_START);
  lv_style_set_pad_all(&screens::rowcontainer, 0);
  // lv_style_set_pad_row(&screens::rowcontainer, 5);
  // lv_style_set_pad_gap(&screens::rowcontainer, 5);
  lv_style_set_border_width(&screens::rowcontainer, 0);

  // Column container
  lv_style_init(&screens::columncontainer);
  lv_style_set_layout(&screens::columncontainer, LV_LAYOUT_FLEX);
  lv_style_set_bg_opa(&screens::columncontainer, LV_OPA_TRANSP);
  lv_style_set_flex_flow(&screens::columncontainer, LV_FLEX_FLOW_COLUMN);
  lv_style_set_size(&screens::columncontainer, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
  lv_style_set_flex_main_place(&screens::columncontainer, LV_FLEX_ALIGN_START);
  lv_style_set_flex_cross_place(&screens::columncontainer, LV_FLEX_ALIGN_START);
  lv_style_set_flex_track_place(&screens::columncontainer, LV_FLEX_ALIGN_START);
  lv_style_set_pad_all(&screens::columncontainer, 0);
  // lv_style_set_pad_column(&screens::columncontainer, 5);
  // lv_style_set_pad_gap(&screens::columncontainer, 5);
  lv_style_set_border_width(&screens::columncontainer, 0);

  // Menu style
  lv_style_init(&screens::menu);
  lv_style_set_bg_color(&screens::menu, current_palette.background);
  lv_style_set_bg_opa(&screens::menu, LV_OPA_COVER);
  lv_style_set_pad_all(&screens::menu, 0);
  lv_style_set_pad_row(&screens::menu, 0);
  lv_style_remove_prop(&screens::menu, LV_OBJ_FLAG_SCROLL_ELASTIC);
  // lv_style_set_size(&screens::menu, lv_pct(100), lv_pct(100));

  // Menu header style
  lv_style_init(&screens::menu_header);
  lv_style_set_bg_color(&screens::menu_header, current_palette.background);
  lv_style_set_bg_opa(&screens::menu_header, LV_OPA_COVER);
  lv_style_set_pad_all(&screens::menu_header, 0);  // LV_DPX(12));
  lv_style_set_height(&screens::menu_header, LV_SIZE_CONTENT);
  // lv_style_set_border_color(&screens::menu_header, current_palette.border);
  // if (current_settings.borders) {
  //   lv_style_set_border_width(&screens::menu_header, 4);
  // } else {
  //   lv_style_set_border_width(&screens::menu_header, 0);
  // }

  // Menu page style
  lv_style_init(&screens::menu_page);
  lv_style_set_bg_color(&screens::menu_page, current_palette.background);
  lv_style_set_bg_opa(&screens::menu_page, LV_OPA_COVER);
  lv_style_set_pad_all(&screens::menu_page, current_settings.element_padding);
  lv_style_remove_prop(&screens::menu_page, LV_OBJ_FLAG_SCROLL_ELASTIC);

  // Menu container
  lv_style_init(&screens::menu_container);
  lv_style_set_bg_opa(&screens::menu_container, LV_OPA_TRANSP);
  // lv_style_set_pad_all(&screens::menu_container, current_settings.element_padding);
  lv_style_set_radius(&screens::menu_container, 8);

  // Menu section
  lv_style_init(&screens::menu_section);
  lv_style_set_bg_color(&screens::menu_section, current_palette.surface);
  lv_style_set_bg_opa(&screens::menu_section, LV_OPA_COVER);
  lv_style_set_pad_all(&screens::menu_section, current_settings.element_padding / 2);
  lv_style_set_pad_row(&screens::menu_section, current_settings.element_padding / 2);
  lv_style_remove_prop(&screens::menu_section, LV_OBJ_FLAG_SCROLL_ELASTIC);

  // Menu background
  lv_style_init(&screens::menu_bg);
  lv_style_set_bg_color(&screens::menu_bg, current_palette.background);
  lv_style_set_bg_opa(&screens::menu_bg, LV_OPA_COVER);
  // lv_style_set_pad_all(&screens::menu_bg, current_settings.element_padding);
  lv_style_set_pad_row(&screens::menu_bg, 4);
  lv_style_remove_prop(&screens::menu_bg, LV_OBJ_FLAG_SCROLL_ELASTIC);

  // Menu selected item
  lv_style_init(&screens::menu_selected);
  lv_style_set_bg_color(&screens::menu_selected, current_palette.primary);
  lv_style_set_bg_opa(&screens::menu_selected, LV_OPA_COVER);
  lv_style_set_text_color(&screens::menu_selected, current_palette.on_primary);
  lv_style_set_radius(&screens::menu_selected, 8);
  lv_style_set_pad_all(&screens::menu_selected, current_settings.element_padding * 1.5);
  lv_style_set_text_font(&screens::menu_selected, &fonts::medium);

  // Menu unselected item
  lv_style_init(&screens::menu_unselected);
  // lv_style_set_bg_color(&screens::menu_unselected, current_palette.surface);
  lv_style_set_bg_opa(&screens::menu_unselected, LV_OPA_TRANSP);
  lv_style_set_text_color(&screens::menu_unselected, current_palette.text);
  lv_style_set_radius(&screens::menu_unselected, 8);
  lv_style_set_pad_all(&screens::menu_unselected, current_settings.element_padding * 1.5);
  lv_style_set_text_font(&screens::menu_unselected, &fonts::medium);

  // Sidebar
  lv_style_init(&screens::sidebar);
  lv_style_set_bg_color(&screens::sidebar, current_palette.surface);
  lv_style_set_bg_opa(&screens::sidebar, LV_OPA_COVER);
  lv_style_set_pad_all(&screens::sidebar, current_settings.element_padding);
  // lv_style_set_size(&screens::sidebar, lv_pct(20), lv_pct(100));

  // Sidebar background
  lv_style_init(&screens::sidebar_bg);
  lv_style_set_bg_color(&screens::sidebar_bg, current_palette.background);
  lv_style_set_bg_opa(&screens::sidebar_bg, LV_OPA_COVER);
  lv_style_set_pad_all(&screens::sidebar_bg, current_settings.element_padding);

  // Sidebar button
  lv_style_init(&screens::sidebar_button);
  lv_style_set_bg_color(&screens::sidebar_button, current_palette.surface);
  lv_style_set_bg_opa(&screens::sidebar_button, LV_OPA_COVER);
  lv_style_set_radius(&screens::sidebar_button, 8);
  lv_style_set_pad_all(&screens::sidebar_button, current_settings.element_padding * 1.5);
  lv_style_set_text_font(&screens::sidebar_button, &fonts::medium);

  // Sidebar button active
  lv_style_init(&screens::sidebar_button_active);
  lv_style_set_bg_color(&screens::sidebar_button_active, current_palette.primary);
  lv_style_set_bg_opa(&screens::sidebar_button_active, LV_OPA_COVER);
  lv_style_set_text_color(&screens::sidebar_button_active, current_palette.on_primary);
  lv_style_set_radius(&screens::sidebar_button_active, 8);
  lv_style_set_pad_all(&screens::sidebar_button_active, current_settings.element_padding * 1.5);
  lv_style_set_text_font(&screens::sidebar_button_active, &fonts::medium);

  // Sidebar button inactive
  lv_style_init(&screens::sidebar_button_inactive);
  lv_style_set_bg_color(&screens::sidebar_button_inactive, current_palette.surface);
  lv_style_set_bg_opa(&screens::sidebar_button_inactive, LV_OPA_COVER);
  lv_style_set_text_color(&screens::sidebar_button_inactive, current_palette.text);
  lv_style_set_radius(&screens::sidebar_button_inactive, 8);
  lv_style_set_pad_all(&screens::sidebar_button_inactive, current_settings.element_padding * 1.5);
  lv_style_set_text_font(&screens::sidebar_button_inactive, &fonts::medium);
}

static void init_text_styles() {
  // Title (largest)
  lv_style_init(&text::title);
  lv_style_set_text_font(&text::title, &fonts::large);
  lv_style_set_text_color(&text::title, current_palette.on_background);

  // Heading
  lv_style_init(&text::heading);
  lv_style_set_text_font(&text::heading, &fonts::medium);
  lv_style_set_text_color(&text::heading, current_palette.on_background);
  lv_style_set_width(&text::heading, lv_pct(100));
  lv_style_set_height(&text::heading, LV_SIZE_CONTENT);

  // Body text
  lv_style_init(&text::body);
  lv_style_set_text_font(&text::body, &fonts::medium);
  lv_style_set_text_color(&text::body, current_palette.text);
  lv_style_set_pad_all(&text::body, current_settings.element_padding / 2);
  lv_style_set_width(&text::body, LV_SIZE_CONTENT);
  // lv_style_set_size(&text::body, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
  // lv_style_set_flex_grow(&text::body, 1);
  // lv_style_set_prop(&text::body, LV_LABEL_LONG_MODE_WRAP, LV_PART_MAIN);

  // Small text
  lv_style_init(&text::small);
  lv_style_set_text_font(&text::small, &fonts::small);
  lv_style_set_text_color(&text::small, lv_palette_lighten(LV_PALETTE_GREY, 1));
  lv_style_set_pad_all(&text::small, current_settings.element_padding / 2);

  // Large numeric value (temperature display)
  lv_style_init(&text::value_large);
  lv_style_set_text_font(&text::value_large, &fonts::numbers_large);
  lv_style_set_text_color(&text::value_large, current_palette.text);
  // lv_style_set_pad_all(&text::value_large, 0);

  // Small numeric value
  lv_style_init(&text::value_small);
  lv_style_set_text_font(&text::value_small, &fonts::numbers_medium);
  lv_style_set_text_color(&text::value_small, current_palette.text);

  // Unit labels (°C, seconds, etc.)
  lv_style_init(&text::unit);
  lv_style_set_text_font(&text::unit, &fonts::numbers_small);
  lv_style_set_text_color(&text::unit, lv_palette_lighten(LV_PALETTE_GREY, 1));

  // Text entry field
  lv_style_init(&text::textentry);
  lv_style_set_bg_color(&text::textentry, current_palette.surface);
  lv_style_set_bg_opa(&text::textentry, LV_OPA_COVER);
  lv_style_set_text_color(&text::textentry, current_palette.text);
  lv_style_set_border_color(&text::textentry, current_palette.border);
  lv_style_set_border_width(&text::textentry, 2);
  lv_style_set_radius(&text::textentry, 8);
  lv_style_set_pad_all(&text::textentry, current_settings.element_padding);

  // Danger/warning text
  lv_style_init(&text::danger);
  lv_style_set_text_color(&text::danger, current_palette.danger);

  // Warning text
  lv_style_init(&text::warning);
  lv_style_set_text_color(&text::warning, current_palette.warning);

  // Success/confirmation text
  lv_style_init(&text::success);
  lv_style_set_text_color(&text::success, current_palette.success);
}

static void init_chart_styles() {
  // Chart background
  lv_style_init(&charts::background);
  lv_style_set_bg_color(&charts::background, current_palette.background);
  lv_style_set_bg_opa(&charts::background, LV_OPA_COVER);
  lv_style_set_border_width(&charts::background, 1);
  lv_style_set_border_color(&charts::background, current_palette.border);
  lv_style_set_pad_all(&charts::background, current_settings.element_padding);

  // Grid lines
  lv_style_init(&charts::grid);
  lv_style_set_line_color(&charts::grid, current_palette.surface);
  // lv_style_set_line_opa(&charts::grid, LV_OPA_30);
  lv_style_set_line_width(&charts::grid, 1);
  lv_style_set_line_dash_width(&charts::grid, 3);
  lv_style_set_line_dash_gap(&charts::grid, 3);

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

  // Set chart indicator size to zero (no visible point markers)
  lv_style_set_size(&charts::indicator, 0, 0);

  // Chart scale (axis labels)
  lv_style_init(&charts::scale);
  lv_style_set_text_color(&charts::scale, current_palette.text);
  lv_style_set_text_font(&charts::scale, &fonts::small);
  lv_style_set_pad_ver(&charts::scale, 10);  // Fixed 10px padding
  lv_style_set_width(&charts::scale, lv_font_get_glyph_width(&fonts::small, '0', '\0') * 6);
  // lv_obj_set_style_pad_ver(_labels->chart_scale_right, lv_chart_get_first_point_center_offset(_labels->chart), 0);
  // lv_obj_set_style_pad_ver(_labels->chart_scale_right, 10, 0);  // Fixed 10px padding
  // lv_obj_set_style_text_font(_labels->chart_scale_right, &lv_font_montserrat_12, 0);
  // lv_font_get_glyph_width(&fonts::small, '0', '\0');
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
  lv_style_set_pad_all(&controls::slider_knob, current_settings.element_padding);
  lv_style_set_border_width(&controls::slider_knob, 2);
  lv_style_set_border_color(&controls::slider_knob, lv_color_white());

  // Switch - background
  lv_style_init(&controls::switch_bg);
  lv_style_set_bg_color(&controls::switch_bg, current_palette.disabled);
  lv_style_set_bg_opa(&controls::switch_bg, LV_OPA_COVER);
  lv_style_set_radius(&controls::switch_bg, LV_RADIUS_CIRCLE);

  // Switch - active indicator
  lv_style_init(&controls::switch_indicator);
  lv_style_set_bg_color(&controls::switch_indicator, current_palette.primary);
  lv_style_set_bg_opa(&controls::switch_indicator, LV_OPA_COVER);

  // Switch - knob
  lv_style_init(&controls::switch_knob);
  lv_style_set_bg_color(&controls::switch_knob, lv_color_white());
  lv_style_set_bg_opa(&controls::switch_knob, LV_OPA_COVER);
  lv_style_set_radius(&controls::switch_knob, LV_RADIUS_CIRCLE);
  lv_style_set_pad_all(&controls::switch_knob, -4);  // TODO: Adjust based on switch size

  // Roller style
  lv_style_init(&controls::roller);
  lv_style_set_bg_color(&controls::roller, current_palette.surface);
  lv_style_set_bg_opa(&controls::roller, LV_OPA_COVER);
  lv_style_set_text_color(&controls::roller, current_palette.text);
  lv_style_set_align(&controls::roller, LV_ALIGN_CENTER);
  lv_style_set_border_color(&controls::roller, current_palette.border);
  lv_style_set_bg_grad_color(&controls::roller, lv_color_darken(current_palette.surface, 128));
  lv_style_set_bg_grad_dir(&controls::roller, LV_GRAD_DIR_VER);
  lv_style_set_text_font(&controls::roller, &fonts::xlarge);

  lv_style_init(&controls::roller_selected);
  lv_style_set_bg_color(&controls::roller_selected, current_palette.primary);
  lv_style_set_bg_opa(&controls::roller_selected, LV_OPA_COVER);

  // Dropdown style
  lv_style_init(&controls::dropdown);
  lv_style_set_bg_color(&controls::dropdown, current_palette.surface);
  lv_style_set_bg_opa(&controls::dropdown, LV_OPA_COVER);
  lv_style_set_text_color(&controls::dropdown, current_palette.text);
  lv_style_set_border_color(&controls::dropdown, current_palette.border);
  lv_style_set_text_font(&controls::dropdown, &fonts::medium);
  lv_style_set_width(&controls::dropdown, LV_SIZE_CONTENT);
  lv_style_set_flex_grow(&controls::dropdown, 1);

  lv_style_init(&controls::dropdown_selected);
  lv_style_set_bg_color(&controls::dropdown_selected, current_palette.primary);
  lv_style_set_bg_opa(&controls::dropdown_selected, LV_OPA_COVER);

  lv_style_init(&controls::dropdown_button);
  lv_style_set_bg_color(&controls::dropdown_button, current_palette.surface);
  lv_style_set_bg_opa(&controls::dropdown_button, LV_OPA_COVER);
}

static void init_indicator_styles() {
  // LED ON state
  lv_style_init(&indicators::led_on);
  lv_style_set_radius(&indicators::led_on, LV_RADIUS_CIRCLE);
  lv_style_set_shadow_width(&indicators::led_on, 10);
  lv_style_set_shadow_color(&indicators::led_on, current_palette.primary);
  lv_style_set_shadow_spread(&indicators::led_on, 5);

  // LED OFF state
  lv_style_init(&indicators::led_off);
  lv_style_set_bg_color(&indicators::led_off, current_palette.disabled);
  lv_style_set_bg_opa(&indicators::led_off, LV_OPA_50);
  lv_style_set_radius(&indicators::led_off, LV_RADIUS_CIRCLE);
  lv_style_set_border_width(&indicators::led_off, 1);
  lv_style_set_border_color(&indicators::led_off, current_palette.disabled);
}

static void init_menu_styles() {
  lv_style_init(&menus::messagebox);
  lv_style_set_bg_color(&menus::messagebox, current_palette.surface);
  lv_style_set_bg_opa(&menus::messagebox, LV_OPA_COVER);
  lv_style_set_radius(&menus::messagebox, 12);
  lv_style_set_pad_all(&menus::messagebox, current_settings.element_padding * 2);
  lv_style_set_border_color(&menus::messagebox, current_palette.border);
  if (current_settings.borders) {
    lv_style_set_border_width(&menus::messagebox, 8);
  } else {
    lv_style_set_border_width(&menus::messagebox, 0);
  }
  lv_style_set_shadow_color(&menus::messagebox, lv_color_black());
  lv_style_set_shadow_opa(&menus::messagebox, LV_OPA_20);

  lv_style_set_bg_color(&menus::messagebox_title, current_palette.on_surface);
  lv_style_set_text_font(&menus::messagebox_title, &fonts::large);
  lv_style_set_text_color(&menus::messagebox_title, current_palette.on_background);

  lv_style_init(&menus::messagebox_message);
  lv_style_set_text_font(&menus::messagebox_message, &fonts::medium);
  lv_style_set_text_color(&menus::messagebox_message, current_palette.text);

  lv_style_init(&menus::messagebox_button);
  lv_style_set_bg_color(&menus::messagebox_button, current_palette.primary);
  lv_style_set_text_color(&menus::messagebox_button, current_palette.on_primary);
  lv_style_set_radius(&menus::messagebox_button, 8);
  lv_style_set_pad_all(&menus::messagebox_button, current_settings.element_padding * 1.5);
  lv_style_set_text_font(&menus::messagebox_button, &fonts::medium);

  // Message box backdrop
  lv_style_init(&menus::msgbox_backdrop);
  lv_style_set_bg_color(&menus::msgbox_backdrop, lv_color_black());
  lv_style_set_bg_opa(&menus::msgbox_backdrop, LV_OPA_50);
}

static void init_debug_styles() {
  lv_style_init(&debug::debug);
  lv_style_set_bg_color(&debug::debug, lv_color_hex(0xFF00FF));
  lv_style_set_bg_opa(&debug::debug, LV_OPA_50);
  lv_style_set_border_color(&debug::debug, lv_color_hex(0x00FFFF));
  lv_style_set_border_width(&debug::debug, 2);
  lv_style_set_text_color(&debug::debug, lv_color_hex(0x00FF00));
}

void Init(ThemeId theme) {
  current_theme = theme;

  lv_display_t* disp = lv_display_get_default();
  uint16_t screen_width = lv_display_get_horizontal_resolution(disp);
  uint16_t screen_height = lv_display_get_vertical_resolution(disp);

  // Load color palette for selected theme
  switch (theme) {
    case ThemeId::TOOTHLESS:
      current_palette = PALETTE_TOOTHLESS;
      current_settings = SETTINGS_TOOTHLESS;
      break;
    case ThemeId::TOOTHLESS_LIGHT:
      current_palette = PALETTE_TOOTHLESS_LIGHT;
      current_settings = SETTINGS_TOOTHLESS;
      break;
    case ThemeId::HIGH_CONTRAST:
      current_palette = PALETTE_HIGH_CONTRAST;
      current_settings = SETTINGS_DEFAULT;
      break;
    case ThemeId::MINIMAL:
      current_palette = PALETTE_MINIMAL;
      current_settings = SETTINGS_DEFAULT;
      break;
    case ThemeId::EMERALD:
      current_palette = PALETTE_EMERALD;
      current_settings = SETTINGS_DEFAULT;
      break;
    case ThemeId::BEELSE:
      current_palette = PALETTE_BEELSE;
      current_settings = SETTINGS_TOOTHLESS;
      break;
    default:
      current_palette = PALETTE_TOOTHLESS;
      current_settings = SETTINGS_TOOTHLESS;
  }

  // Select fonts based on detected siz
  if (screen_width == 640 && screen_height == 180) {
    FLOG_INFO("LONG Screen detected (%ux%u)", screen_width, screen_height);
    fonts::tiny = lv_font_montserrat_12;
    fonts::small = lv_font_montserrat_14;
    fonts::medium = lv_font_montserrat_16;
    fonts::large = lv_font_montserrat_18;
    fonts::xlarge = lv_font_montserrat_22;
    fonts::numbers_small = AdwaitaMonoB_28;
    fonts::numbers_medium = AdwaitaMonoB_48;
    fonts::numbers_large = AdwaitaMonoB_128;
    current_settings.borders = false;
    current_settings.screen_padding = 0;
    current_settings.button_height = LV_DPX(20);
    current_settings.element_padding = LV_DPX(4);
  } else if (screen_width <= 320) {
    FLOG_INFO("Small screen detected (%ux%u), using small fonts", screen_width, screen_height);
    fonts::tiny = lv_font_montserrat_8;
    fonts::small = lv_font_montserrat_12;
    fonts::medium = lv_font_montserrat_14;
    fonts::large = lv_font_montserrat_16;
    fonts::xlarge = lv_font_montserrat_18;
    fonts::numbers_small = AdwaitaMonoB_28;
    fonts::numbers_medium = AdwaitaMonoB_32;
    fonts::numbers_large = AdwaitaMonoB_96;
    current_settings.button_height = LV_DPX(30);
    current_settings.element_padding = LV_DPX(4);
  } else if (screen_width <= 480) {
    FLOG_INFO("Medium screen detected (%ux%u), using medium fonts", screen_width, screen_height);
    fonts::tiny = lv_font_montserrat_10;
    fonts::small = lv_font_montserrat_16;
    fonts::medium = lv_font_montserrat_20;
    fonts::large = lv_font_montserrat_24;
    fonts::xlarge = lv_font_montserrat_32;
    fonts::numbers_small = AdwaitaMonoB_28;
    fonts::numbers_medium = AdwaitaMonoB_32;
    fonts::numbers_large = AdwaitaMonoB_48;
  } else {
    FLOG_INFO("Large screen detected (%ux%u), using large fonts", screen_width, screen_height);
    fonts::tiny = lv_font_montserrat_12;
    fonts::small = lv_font_montserrat_18;
    fonts::medium = lv_font_montserrat_22;
    fonts::large = lv_font_montserrat_32;
    fonts::xlarge = lv_font_montserrat_48;
    fonts::numbers_small = AdwaitaMonoB_28;
    fonts::numbers_medium = AdwaitaMonoB_64;
    fonts::numbers_large = AdwaitaMonoB_128;
    current_settings.button_height = LV_DPX(60);
    current_settings.element_padding = LV_DPX(12);
    // if (current_settings.screen_padding > 0) {
    //   current_settings.screen_padding = LV_DPX(32);
    // }
    // current_settings.screen_padding = LV_DPX(24);
  }

  // Initialize all style categories
  init_button_styles();
  init_screen_styles();
  init_text_styles();
  init_chart_styles();
  init_control_styles();
  init_indicator_styles();
  init_menu_styles();

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

  lv_style_reset(&menus::messagebox);
  lv_style_reset(&menus::messagebox_title);
  lv_style_reset(&menus::messagebox_message);
  lv_style_reset(&menus::messagebox_button);
  lv_style_reset(&menus::msgbox_backdrop);

  lv_style_reset(&screens::background);
  lv_style_reset(&screens::card);
  lv_style_reset(&screens::subscreen);
  lv_style_reset(&screens::container);
  lv_style_reset(&screens::rowcontainer);
  lv_style_reset(&screens::columncontainer);
  lv_style_reset(&screens::menu);
  lv_style_reset(&screens::menu_header);
  lv_style_reset(&screens::menu_page);
  lv_style_reset(&screens::menu_container);
  lv_style_reset(&screens::menu_section);
  lv_style_reset(&screens::menu_selected);
  lv_style_reset(&screens::menu_unselected);
  lv_style_reset(&screens::menu_bg);
  lv_style_reset(&screens::sidebar);
  lv_style_reset(&screens::sidebar_bg);
  lv_style_reset(&screens::sidebar_button);
  lv_style_reset(&screens::sidebar_button_active);
  lv_style_reset(&screens::sidebar_button_inactive);

  lv_style_reset(&text::title);
  lv_style_reset(&text::heading);
  lv_style_reset(&text::body);
  lv_style_reset(&text::small);
  lv_style_reset(&text::value_large);
  lv_style_reset(&text::value_small);
  lv_style_reset(&text::unit);
  lv_style_reset(&text::textentry);

  lv_style_reset(&charts::background);
  lv_style_reset(&charts::grid);
  lv_style_reset(&charts::line_temp);
  lv_style_reset(&charts::line_target);
  lv_style_reset(&charts::cursor);
  lv_style_reset(&charts::indicator);
  lv_style_reset(&charts::scale);

  lv_style_reset(&controls::slider_main);
  lv_style_reset(&controls::slider_indicator);
  lv_style_reset(&controls::slider_knob);
  lv_style_reset(&controls::switch_bg);
  lv_style_reset(&controls::switch_indicator);
  lv_style_reset(&controls::switch_knob);
  lv_style_reset(&controls::roller);
  lv_style_reset(&controls::dropdown);
  lv_style_reset(&controls::dropdown_selected);
  lv_style_reset(&controls::dropdown_button);

  lv_style_reset(&indicators::led_on);
  lv_style_reset(&indicators::led_off);

  lv_style_reset(&debug::debug);

  // Re-initialize with new theme
  Init(new_theme);

  // Trigger style refresh on all existing objects
  lv_obj_report_style_change(NULL);
}

const ColorPalette& GetColors() { return current_palette; }

ThemeId GetCurrentTheme() { return current_theme; }

}  // namespace toothless::themes
