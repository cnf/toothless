/// @file style_registry.hpp
/// @brief Global style registry for consistent theming
#pragma once

#include <lvgl.h>

#include "theme_config.hpp"

namespace toothless::themes {

/// Button styles
namespace buttons {
extern lv_style_t primary;    ///< Primary action button (Start, Confirm, etc.)
extern lv_style_t secondary;  ///< Secondary action button
extern lv_style_t danger;     ///< Dangerous action button (Stop, Delete, etc.)
extern lv_style_t success;    ///< Success/confirmation button
extern lv_style_t settings;   ///< Settings/gear icon button
extern lv_style_t pressed;    ///< Pressed state for all buttons
extern lv_style_t disabled;   ///< Disabled state for all buttons
}  // namespace buttons

/// Screen and container styles
namespace screens {
extern lv_style_t background;       ///< Main screen background
extern lv_style_t subscreen;        ///< Sub-screen (transparent overlay)
extern lv_style_t card;             ///< Card/panel surface
extern lv_style_t rowcontainer;     ///< Row container
extern lv_style_t columncontainer;  ///< Column container
extern lv_style_t menu_bg;          ///< Menu background
}  // namespace screens

/// Text/label styles
namespace text {
extern lv_style_t title;        ///< Large title text (24pt)
extern lv_style_t heading;      ///< Section heading (20pt)
extern lv_style_t body;         ///< Normal body text (16pt)
extern lv_style_t small;        ///< Small text (12pt)
extern lv_style_t value_large;  ///< Large numeric value display (48pt)
extern lv_style_t value_small;  ///< Small numeric value (24pt)
extern lv_style_t unit;         ///< Unit label (°C, sec, etc.)
}  // namespace text

/// Chart styles
namespace charts {
extern lv_style_t background;   ///< Chart background
extern lv_style_t grid_lines;   ///< Grid line style
extern lv_style_t line_temp;    ///< Temperature line color
extern lv_style_t line_target;  ///< Target temperature line
extern lv_style_t cursor;       ///< Chart cursor
extern lv_style_t indicator;    ///< Chart indicator style
}  // namespace charts

/// Control styles (sliders, switches, etc.)
namespace controls {
extern lv_style_t slider_main;       ///< Slider track
extern lv_style_t slider_indicator;  ///< Slider filled part
extern lv_style_t slider_knob;       ///< Slider handle
extern lv_style_t switch_bg;         ///< Switch background
extern lv_style_t switch_indicator;  ///< Switch active indicator
extern lv_style_t switch_knob;       ///< Switch knob/handle
extern lv_style_t roller;            ///< Roller widget style
extern lv_style_t roller_selected;   ///< Roller selected item style
}  // namespace controls

/// LED indicator styles
namespace indicators {
extern lv_style_t led_on;   ///< LED indicator ON state
extern lv_style_t led_off;  ///< LED indicator OFF state
}  // namespace indicators

namespace menus {
extern lv_style_t messagebox;
extern lv_style_t messagebox_backdrop;
extern lv_style_t messagebox_title;
extern lv_style_t messagebox_message;
extern lv_style_t messagebox_button;
extern lv_style_t menu_background;  ///< Menu background style
}  // namespace menus

namespace fonts {
extern lv_font_t tiny;
extern lv_font_t small;
extern lv_font_t medium;
extern lv_font_t large;
extern lv_font_t xlarge;
extern lv_font_t numbers_small;
extern lv_font_t numbers_medium;
extern lv_font_t numbers_large;
}  // namespace fonts

/// Initialize all styles with specified theme
/// @param theme Theme ID to initialize
void Init(ThemeId theme = ThemeId::REFLOW_DARK);

/// Switch to a different theme at runtime
/// @param new_theme New theme to apply
void SwitchTheme(ThemeId new_theme);

/// Get current color palette
/// @return Reference to active color palette
const ColorPalette& GetColors();

/// Get current theme ID
/// @return Active theme ID
ThemeId GetCurrentTheme();

}  // namespace toothless::themes
