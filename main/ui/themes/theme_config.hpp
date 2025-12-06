/// @file theme_config.hpp
/// @brief Theme color palette and configuration
#pragma once

#include <lvgl.h>

#include <string>
#include <utility>

#include "config_mgr.hpp"

namespace toothless::themes {

/// Color palette for a theme
struct ColorPalette {
  lv_color_t primary;        ///< Primary accent color (e.g., buttons, highlights)
  lv_color_t secondary;      ///< Secondary accent color
  lv_color_t background;     ///< Main screen background
  lv_color_t surface;        ///< Cards, panels, elevated surfaces
  lv_color_t text;           ///< General text color
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

struct Settings {
  bool borders;
  uint32_t screen_padding;             ///< Standard screen padding
  size_t element_padding = LV_DPX(8);  ///< Standard element padding
  size_t button_height = 60;           ///< Standard button height
};

/// Available theme IDs
enum class ThemeId {
  TOOTHLESS,        ///< Dark theme for reflow oven (default)
  TOOTHLESS_LIGHT,  ///< Light theme for reflow oven
  BEELSE,           ///< Beelse green theme
  EMERALD,          ///< Emerald green theme
  HIGH_CONTRAST,    ///< High contrast for visibility
  MINIMAL           ///< Minimal monochrome theme
};

static constexpr std::pair<ThemeId, const char*> kThemeMap[] = {
    {ThemeId::TOOTHLESS, "Toothless"},              //
    {ThemeId::TOOTHLESS_LIGHT, "Toothless Light"},  //
    {ThemeId::BEELSE, "Beelse"},                    //
    {ThemeId::EMERALD, "Emerald"},                  //
    {ThemeId::HIGH_CONTRAST, "High Contrast"},      //
    {ThemeId::MINIMAL, "Minimal"},                  //
};

inline std::string ToString(ThemeId t) {
  for (auto& [theme, name] : kThemeMap)
    if (theme == t) return name;
  return "unknown";
}

// inline ThemeId FromString(const std::string& s) {
//   for (auto& [theme, name] : kThemeMap)
//     if (s == name) return theme;
//   return ThemeId::TOOTHLESS;
// };
inline ThemeId FromString(const std::string& name) {
  // auto normalize = [](const std::string& s) {
  //   std::string result;
  //   for (char c : s) {
  //     if (c == ' ' || c == '-')
  //       result += '_';
  //     else
  //       result += std::toupper(c);
  //   }
  //   return result;
  // };

  // std::string normalized = normalize(name);
  std::string normalized = config_utils::NormalizeString(name);

  for (const auto& [id, theme_name] : kThemeMap) {
    if (normalized == config_utils::NormalizeString(theme_name)) {
      return id;
    }
  }

  return ThemeId::TOOTHLESS_LIGHT;
}

inline std::string MakeFormat() {
  std::string f = "enum=";
  for (size_t i = 0; i < std::size(kThemeMap); ++i) {
    if (i > 0) f += "|";
    f += kThemeMap[i].second;
  }
  return f;
}

/*
// Load:
HeaterMode mode = StringToMode(std::get<std::string>(settings["mode"]));

// Save:
PS_PUB_STR(topics::heater::mode_set, ModeToString(mode));
*/

// Theme palette definitions
static const ColorPalette PALETTE_TOOTHLESS = {
    .primary = lv_color_hex(0xFF6700),        // Orange for heat/temperature
    .secondary = lv_color_hex(0x0098FF),      // Blue for cooling
    .background = lv_color_black(),           // Pure black background [old 0x001122]
    .surface = lv_color_hex(0x001122),        // Slightly lighter surface
    .text = lv_color_hex(0xE0E0E0),           // Light gray text
    .on_primary = lv_color_white(),           // White text on orange
    .on_secondary = lv_color_white(),         // White text on blue
    .on_background = lv_color_hex(0xE0E0E0),  // Light gray text
    .on_surface = lv_color_hex(0xE0E0E0),     // Light gray text
    .danger = lv_color_hex(0xFF0000),         // Red for stop/danger
    .success = lv_color_hex(0x00CC66),        // Green for success
    .warning = lv_color_hex(0xFFAA00),        // Amber for warnings
    .disabled = lv_color_hex(0x555555),       // Gray for disabled
    .border = lv_color_hex(0x001727),         // Subtle border
};

static const Settings SETTINGS_DEFAULT = {
    .borders = true,
    .screen_padding = LV_DPX(10)  // Standard screen padding
};

static const Settings SETTINGS_TOOTHLESS = {
    .borders = false,
    .screen_padding = 0  // LV_DPX(5)  // Standard screen padding
};

static const ColorPalette PALETTE_TOOTHLESS_LIGHT = {
    .primary = lv_color_hex(0xFF6700),        // Orange
    .secondary = lv_color_hex(0x0099FF),      // Blue
    .background = lv_color_white(),           // Pure white background
    .surface = lv_color_hex(0xF5F5F5),        // Light gray surface
    .text = lv_color_hex(0x212121),           // Dark gray text
    .on_primary = lv_color_white(),           // White on orange
    .on_secondary = lv_color_white(),         // White on blue
    .on_background = lv_color_hex(0x212121),  // Dark gray text
    .on_surface = lv_color_hex(0x212121),     // Dark gray text
    .danger = lv_color_hex(0xD32F2F),         // Dark red
    .success = lv_color_hex(0x388E3C),        // Dark green
    .warning = lv_color_hex(0xF57C00),        // Dark amber
    .disabled = lv_color_hex(0xBDBDBD),       // Light gray
    .border = lv_color_hex(0xE0E0E0)          // Light gray border
};

static const ColorPalette PALETTE_HIGH_CONTRAST = {
    .primary = lv_color_hex(0xFFFF00),    // Bright yellow
    .secondary = lv_color_hex(0x00FFFF),  // Cyan
    .background = lv_color_black(),       // Pure black
    .surface = lv_color_hex(0x1a1a1a),    // Very dark gray
    .text = lv_color_white(),             // Pure white text
    .on_primary = lv_color_black(),       // Black on yellow
    .on_secondary = lv_color_black(),     // Black on cyan
    .on_background = lv_color_white(),    // Pure white text
    .on_surface = lv_color_white(),       // Pure white text
    .danger = lv_color_hex(0xFF0000),     // Pure red
    .success = lv_color_hex(0x00FF00),    // Pure green
    .warning = lv_color_hex(0xFFAA00),    // Orange
    .disabled = lv_color_hex(0x666666),   // Mid gray
    .border = lv_color_hex(0x666666)      // Mid gray border
};

static const ColorPalette PALETTE_MINIMAL = {
    .primary = lv_color_hex(0x333333),
    .secondary = lv_color_hex(0x666666),
    .background = lv_color_white(),
    .surface = lv_color_hex(0xF8F8F8),
    .text = lv_color_black(),
    .on_primary = lv_color_black(),
    .on_secondary = lv_color_black(),
    .on_background = lv_color_black(),
    .on_surface = lv_color_black(),
    .danger = lv_color_hex(0x000000),
    .success = lv_color_hex(0x000000),
    .warning = lv_color_hex(0x666666),
    .disabled = lv_color_hex(0xCCCCCC),
    .border = lv_color_hex(0xDDDDDD)  //
};

static const ColorPalette PALETTE_EMERALD = {
    .primary = lv_color_hex(0x10B981),        // Emerald green - main accent
    .secondary = lv_color_hex(0x059669),      // Darker emerald - secondary actions
    .background = lv_color_hex(0x0F1419),     // Very dark slate/black
    .surface = lv_color_hex(0x1A2128),        // Dark slate surface
    .text = lv_color_hex(0xE0E0E0),           // Light gray text
    .on_primary = lv_color_white(),           // White text on emerald
    .on_secondary = lv_color_white(),         // White text on dark emerald
    .on_background = lv_color_hex(0xE0E0E0),  // Light gray text
    .on_surface = lv_color_hex(0xE0E0E0),     // Light gray text
    .danger = lv_color_hex(0xEF4444),         // Red for stop/danger
    .success = lv_color_hex(0x10B981),        // Emerald for success (same as primary)
    .warning = lv_color_hex(0xF59E0B),        // Amber for warnings
    .disabled = lv_color_hex(0x4B5563),       // Gray for disabled
    .border = lv_color_hex(0x1F2937)          // Subtle dark border
};

static const ColorPalette PALETTE_BEELSE = {
    .primary = lv_color_hex(0x25a821),        // Green
    .secondary = lv_color_hex(0xf7da15),      // Darker green
    .background = lv_color_hex(0x0C1314),     // Very dark cyan-black
    .surface = lv_color_hex(0x1A2527),        // Dark cyan surface
    .text = lv_color_hex(0xE0E0E0),           // Light gray text
    .on_primary = lv_color_white(),           // Black on green for contrast
    .on_secondary = lv_color_white(),         // White on yellow-green
    .on_background = lv_color_hex(0xE0E0E0),  // Light gray text
    .on_surface = lv_color_hex(0xE0E0E0),     // Light gray text
    .danger = lv_color_hex(0xFF3B30),         // Coral red
    .success = lv_color_hex(0x14B8A6),        // Jade (same as primary)
    .warning = lv_color_hex(0xFFCC00),        // Gold
    .disabled = lv_color_hex(0x475569),       // Gray for disabled
    .border = lv_color_hex(0x1E2D30)          // Subtle dark border
};

// 004225
}  // namespace toothless::themes
