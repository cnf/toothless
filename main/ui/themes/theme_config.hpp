/// @file theme_config.hpp
/// @brief Theme color palette and configuration
#pragma once

#include <lvgl.h>

namespace toothless::themes {

/// Color palette for a theme
struct ColorPalette {
  lv_color_t primary;        ///< Primary accent color (e.g., buttons, highlights)
  lv_color_t secondary;      ///< Secondary accent color
  lv_color_t background;     ///< Main screen background
  lv_color_t surface;        ///< Cards, panels, elevated surfaces
  lv_color_t on_primary;     ///< Text/icons on primary color
  lv_color_t on_secondary;   ///< Text/icons on secondary color
  lv_color_t on_background;  ///< Text/icons on background
  lv_color_t on_surface;     ///< Text/icons on surface
  lv_color_t danger;         ///< Error, stop, danger actions
  lv_color_t success;        ///< Success, go, confirmation
  lv_color_t warning;        ///< Warning, caution
  lv_color_t disabled;       ///< Disabled elements
  lv_color_t border;         ///< Borders, dividers
};

/// Available theme IDs
enum class ThemeId {
  REFLOW_DARK,    ///< Dark theme for reflow oven (default)
  REFLOW_LIGHT,   ///< Light theme for reflow oven
  HIGH_CONTRAST,  ///< High contrast for visibility
  MINIMAL         ///< Minimal monochrome theme
};

// Theme palette definitions
static constexpr ColorPalette PALETTE_REFLOW_DARK = {
    .primary = lv_color_hex(0xFF6700),        // Orange for heat/temperature
    .secondary = lv_color_hex(0x0098FF),      // Blue for cooling
    .background = lv_color_hex(0x001122),     // Very dark blue-black
    .surface = lv_color_hex(0x1a2332),        // Slightly lighter surface
    .on_primary = lv_color_white(),           // White text on orange
    .on_secondary = lv_color_white(),         // White text on blue
    .on_background = lv_color_hex(0xE0E0E0),  // Light gray text
    .on_surface = lv_color_hex(0xE0E0E0),     // Light gray text
    .danger = lv_color_hex(0xFF3333),         // Red for stop/danger
    .success = lv_color_hex(0x00CC66),        // Green for success
    .warning = lv_color_hex(0xFFAA00),        // Amber for warnings
    .disabled = lv_color_hex(0x555555),       // Gray for disabled
    .border = lv_color_hex(0x3a4352)          // Subtle border
};

static constexpr ColorPalette PALETTE_REFLOW_LIGHT = {.primary = lv_color_hex(0xFF6700),
                                                      .secondary = lv_color_hex(0x0099FF),
                                                      .background = lv_color_white(),
                                                      .surface = lv_color_hex(0xF5F5F5),
                                                      .on_primary = lv_color_white(),
                                                      .on_secondary = lv_color_white(),
                                                      .on_background = lv_color_hex(0x212121),
                                                      .on_surface = lv_color_hex(0x212121),
                                                      .danger = lv_color_hex(0xD32F2F),
                                                      .success = lv_color_hex(0x388E3C),
                                                      .warning = lv_color_hex(0xF57C00),
                                                      .disabled = lv_color_hex(0xBDBDBD),
                                                      .border = lv_color_hex(0xE0E0E0)};

static constexpr ColorPalette PALETTE_HIGH_CONTRAST = {.primary = lv_color_hex(0xFFFF00),    // Bright yellow
                                                       .secondary = lv_color_hex(0x00FFFF),  // Cyan
                                                       .background = lv_color_black(),       // Pure black
                                                       .surface = lv_color_hex(0x1a1a1a),    // Very dark gray
                                                       .on_primary = lv_color_black(),       // Black on yellow
                                                       .on_secondary = lv_color_black(),     // Black on cyan
                                                       .on_background = lv_color_white(),    // Pure white text
                                                       .on_surface = lv_color_white(),       // Pure white text
                                                       .danger = lv_color_hex(0xFF0000),     // Pure red
                                                       .success = lv_color_hex(0x00FF00),    // Pure green
                                                       .warning = lv_color_hex(0xFFAA00),    // Orange
                                                       .disabled = lv_color_hex(0x666666),   // Mid gray
                                                       .border = lv_color_hex(0x666666)};

static constexpr ColorPalette PALETTE_MINIMAL = {.primary = lv_color_hex(0x333333),
                                                 .secondary = lv_color_hex(0x666666),
                                                 .background = lv_color_white(),
                                                 .surface = lv_color_hex(0xF8F8F8),
                                                 .on_primary = lv_color_white(),
                                                 .on_secondary = lv_color_white(),
                                                 .on_background = lv_color_black(),
                                                 .on_surface = lv_color_black(),
                                                 .danger = lv_color_hex(0x000000),
                                                 .success = lv_color_hex(0x000000),
                                                 .warning = lv_color_hex(0x666666),
                                                 .disabled = lv_color_hex(0xCCCCCC),
                                                 .border = lv_color_hex(0xDDDDDD)};

}  // namespace toothless::themes
