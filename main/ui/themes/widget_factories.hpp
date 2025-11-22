/// @file widget_factories.hpp
/// @brief Pre-styled widget factory functions
#pragma once

#include <lvgl.h>

namespace toothless::ui {

// ============================================================================
// BUTTON FACTORIES
// ============================================================================

/// Create a primary action button (Start, Confirm, etc.)
/// @param parent Parent object
/// @param text Button label text
/// @param grow If true, button grows to fill available space
/// @return Created button object
lv_obj_t* CreatePrimaryButton(lv_obj_t* parent, const char* text, bool grow = false);

/// Create a secondary action button
/// @param parent Parent object
/// @param text Button label text
/// @param grow If true, button grows to fill available space
/// @return Created button object
lv_obj_t* CreateSecondaryButton(lv_obj_t* parent, const char* text, bool grow = false);

/// Create a danger/stop button
/// @param parent Parent object
/// @param text Button label text (e.g., "Stop", "Delete")
/// @param grow If true, button grows to fill available space
/// @return Created button object
lv_obj_t* CreateDangerButton(lv_obj_t* parent, const char* text, bool grow = false);

/// Create a success/go button
/// @param parent Parent object
/// @param text Button label text
/// @param grow If true, button grows to fill available space
/// @return Created button object
lv_obj_t* CreateSuccessButton(lv_obj_t* parent, const char* text, bool grow = false);

/// Create a settings icon button
/// @param parent Parent object
/// @return Created button object with gear icon
lv_obj_t* CreateSettingsButton(lv_obj_t* parent);

// ============================================================================
// SCREEN & CONTAINER FACTORIES
// ============================================================================

/// Create a new screen with themed background
/// @return Created screen object
lv_obj_t* CreateScreen();

/// Create a card/panel container
/// @param parent Parent object
/// @return Created card object
lv_obj_t* CreateCard(lv_obj_t* parent);

/// Create a menu background container
/// @param parent Parent object
/// @return Created menu container
lv_obj_t* CreateMenuContainer(lv_obj_t* parent);

// ============================================================================
// TEXT/LABEL FACTORIES
// ============================================================================

/// Create a title label (largest text)
/// @param parent Parent object
/// @param text Label text
/// @return Created label object
lv_obj_t* CreateTitle(lv_obj_t* parent, const char* text);

/// Create a heading label
/// @param parent Parent object
/// @param text Label text
/// @return Created label object
lv_obj_t* CreateHeading(lv_obj_t* parent, const char* text);

/// Create a body text label
/// @param parent Parent object
/// @param text Label text
/// @return Created label object
lv_obj_t* CreateBodyText(lv_obj_t* parent, const char* text);

/// Create a small text label
/// @param parent Parent object
/// @param text Label text
/// @return Created label object
lv_obj_t* CreateSmallText(lv_obj_t* parent, const char* text);

/// Create a large numeric value display (e.g., temperature)
/// @param parent Parent object
/// @param value Initial value
/// @param format Printf-style format string (e.g., "%.1f")
/// @return Created label object
lv_obj_t* CreateValueLarge(lv_obj_t* parent, float value, const char* format = "%.0f");

/// Create a small numeric value display
/// @param parent Parent object
/// @param value Initial value
/// @param format Printf-style format string
/// @return Created label object
lv_obj_t* CreateValueSmall(lv_obj_t* parent, int value, const char* format = "%d");

/// Create a unit label (°C, sec, etc.)
/// @param parent Parent object
/// @param unit Unit text
/// @return Created label object
lv_obj_t* CreateUnitLabel(lv_obj_t* parent, const char* unit);

// ============================================================================
// CONTROL FACTORIES
// ============================================================================

/// Create a styled slider
/// @param parent Parent object
/// @param min Minimum value
/// @param max Maximum value
/// @param value Initial value
/// @return Created slider object
lv_obj_t* CreateSlider(lv_obj_t* parent, int32_t min, int32_t max, int32_t value);

/// Create a styled switch
/// @param parent Parent object
/// @param initial_state Initial ON/OFF state
/// @return Created switch object
lv_obj_t* CreateSwitch(lv_obj_t* parent, bool initial_state = false);

// ============================================================================
// CHART FACTORIES
// ============================================================================

/// Create a styled line chart for temperature display
/// @param parent Parent object
/// @param points_capacity Maximum number of data points
/// @return Created chart object
lv_obj_t* CreateTemperatureChart(lv_obj_t* parent, uint16_t points_capacity);

// ============================================================================
// INDICATOR FACTORIES
// ============================================================================

/// Create an LED indicator
/// @param parent Parent object
/// @param initial_state Initial ON/OFF state
/// @return Created LED object
lv_obj_t* CreateLEDIndicator(lv_obj_t* parent, bool initial_state = false);

// ============================================================================
// HELPER FUNCTIONS
// ============================================================================

/// Update a value label
/// @param label Label object created by CreateValue* functions
/// @param value New value
/// @param format Printf-style format string
void UpdateValueLabel(lv_obj_t* label, float value, const char* format = "%.0f");

/// Set LED indicator state
/// @param led LED object created by CreateLEDIndicator
/// @param on True for ON, false for OFF
void SetLEDState(lv_obj_t* led, bool on);

}  // namespace toothless::ui
