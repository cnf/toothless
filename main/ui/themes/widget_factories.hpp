/// @file widget_factories.hpp
/// @brief Pre-styled widget factory functions
#pragma once

#include <lvgl.h>

#include <functional>
#include <string>

namespace toothless::ui {

// ============================================================================
// BUTTON FACTORIES
// ============================================================================

/// @brief  Create a primary action button (Start, Confirm, etc.)
/// @param parent Parent object
/// @param text Button label text
/// @param width width
/// @param height height
/// @param grow If true, button grows to fill available space
/// @return Created button object
lv_obj_t* CreatePrimaryButton(lv_obj_t* parent, const char* text, int32_t width, int32_t height, bool grow);

/// Create a primary action button (Start, Confirm, etc.)
/// @param parent Parent object
/// @param text Button label text
/// @param grow If true, button grows to fill available space
/// @return Created button object
lv_obj_t* CreatePrimaryButton(lv_obj_t* parent, const char* text, bool grow = false);

/// @brief  Create a secondary action button
/// @param parent Parent object
/// @param text Button label text
/// @param width  width
/// @param height height
/// @param grow If true, button grows to fill available space
/// @return Created button object
lv_obj_t* CreateSecondaryButton(lv_obj_t* parent, const char* text, int32_t width, int32_t height, bool grow);

/// Create a secondary action button
/// @param parent Parent object
/// @param text Button label text
/// @param grow If true, button grows to fill available space
/// @return Created button object
lv_obj_t* CreateSecondaryButton(lv_obj_t* parent, const char* text, bool grow = false);

/// Create a danger/stop button
/// @param parent Parent object
/// @param text Button label text (e.g., "Stop", "Delete")
/// @param width width
/// @param height height
/// @param grow If true, button grows to fill available space
/// @return Created button object
lv_obj_t* CreateDangerButton(lv_obj_t* parent, const char* text, int32_t width, int32_t height, bool grow);

/// Create a danger/stop button
/// @param parent Parent object
/// @param text Button label text (e.g., "Stop", "Delete")
/// @param grow If true, button grows to fill available space
/// @return Created button object
lv_obj_t* CreateDangerButton(lv_obj_t* parent, const char* text, bool grow = false);

/// Create a success/go button
/// @param parent Parent object
/// @param text Button label text
/// @param width width
/// @param height height
/// @param grow If true, button grows to fill available space
/// @return Created button object
lv_obj_t* CreateSuccessButton(lv_obj_t* parent, const char* text, int32_t width, int32_t height, bool grow);

/// Create a success/go button
/// @param parent Parent object
/// @param text Button label text
/// @param grow If true, button grows to fill available space
/// @return Created button object
lv_obj_t* CreateSuccessButton(lv_obj_t* parent, const char* text, bool grow = false);

/// @brief  Create a settings icon button
/// @param parent Parent object
/// @param width width
/// @param height height
/// @param grow If true, button grows to fill available space
/// @return Created button object with gear icon
lv_obj_t* CreateSettingsButton(lv_obj_t* parent, int32_t width, int32_t height, bool grow);

/// Create a settings icon button
/// @param parent Parent object
/// @param grow If true, button grows to fill available space
/// @return Created button object with gear icon
lv_obj_t* CreateSettingsButton(lv_obj_t* parent, bool grow = false);

// ============================================================================
// SCREEN & CONTAINER FACTORIES
// ============================================================================

/// Create a new screen with themed background
/// @return Created screen object
lv_obj_t* CreateScreen();

/// @brief Create a sub-screen container, for split screen layouts
/// @param parent Parent object
/// @return Created sub-screen object
lv_obj_t* CreateSubScreen(lv_obj_t* parent);

/// Create a card/panel container
/// @param parent Parent object
/// @return Created card object
lv_obj_t* CreateCard(lv_obj_t* parent);

/// Create a generic container
/// @param parent Parent object
/// @return Created container object
lv_obj_t* CreateContainer(lv_obj_t* parent);

/// Create a row container for horizontal layouts
/// @param parent Parent object
/// @return Created row container
lv_obj_t* CreateRowContainer(lv_obj_t* parent);

/// Create a column container for vertical layouts
/// @param parent Parent object
/// @return Created column container
lv_obj_t* CreateColumnContainer(lv_obj_t* parent);

/// Create a menu background container
/// @param parent Parent object
/// @return Created menu container
lv_obj_t* CreateMenuContainer(lv_obj_t* parent);

/// Create a menu with title
/// @param parent Parent object
/// @param title Menu title text
/// @return Created menu object
lv_obj_t* CreateMenu(lv_obj_t* parent, const char* title = NULL);

/// Create a menu page with title
/// @param parent Parent object
/// @param title Menu page title text
/// @return Created menu page object
lv_obj_t* CreateMenuPage(lv_obj_t* parent, const char* title = NULL);

/// Create a menu section container
/// @param parent Parent object
/// @return Created menu section object
lv_obj_t* CreateMenuSection(lv_obj_t* parent);

/// @brief  Create a sidebar page and set it as the current sidebar page
/// @param parent Parent menu object
/// @param title Sidebar page title text
/// @return Created sidebar page object
lv_obj_t* CreateMenuRootPage(lv_obj_t* parent, const char* title = NULL);

/// Create a sidebar section container
/// @param parent Parent object
/// @return Created sidebar section object
lv_obj_t* CreateMenuRootSection(lv_obj_t* parent);

/// Create a menu sidebar entry with optional icon
/// @param parent Parent object
/// @param title Entry title text
/// @param icon Optional icon source (file path, symbol, etc.)
/// @return Created sidebar entry object
lv_obj_t* CreateMenuRootEntry(lv_obj_t* parent, const char* title, const char* icon = NULL);

/// @brief Create a menu sidebar entry with custom object and optional icon
/// @param parent Parent object
/// @param obj Custom object to use as the entry label
/// @param icon Optional icon source (file path, symbol, etc.)
/// @return Created sidebar entry object
lv_obj_t* CreateMenuRootEntry(lv_obj_t* parent, lv_obj_t* obj, const char* icon);

/// Create and style a menu sidebar
/// @param menu Menu object
/// @return Created sidebar object
lv_obj_t* StyleMenuSidebar(lv_obj_t* menu);

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
lv_obj_t* CreateValueSmall(lv_obj_t* parent, float value, const char* format = "%li");

/// Create a unit label (°C, sec, etc.)
/// @param parent Parent object
/// @param unit Unit text
/// @return Created label object
lv_obj_t* CreateUnitLabel(lv_obj_t* parent, const char* unit);

/// Create a styled text area for text input
/// @param parent Parent object
/// @return Created text area object
lv_obj_t* CreateTextArea(lv_obj_t* parent);

/// Create a single-line text area for text input
/// @param parent Parent object
/// @return Created single-line text area object
lv_obj_t* CreateTextLine(lv_obj_t* parent);

/// Create an icon item with optional text
/// @param parent Parent object
/// @param txt Optional text label
/// @param icon Icon source (file path, symbol, etc.)
/// @return Created icon item object
lv_obj_t* CreateIconItem(lv_obj_t* parent, const char* txt, const char* icon = NULL);

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

/// Create a styled roller widget
/// @param parent Parent object
/// @param options Options string (newline-separated)
/// @param selected Initially selected option index
/// @return Created roller object
lv_obj_t* CreateRoller(lv_obj_t* parent, const char* options, int32_t selected);

/// Create a small styled roller widget
/// @param parent Parent object
/// @param options Options string (newline-separated)
/// @param selected Initially selected option index
/// @return Created small roller object
lv_obj_t* CreateSmallRoller(lv_obj_t* parent, const char* options, int32_t selected);

/// @brief Create a styled dropdown widget
/// @param parent Parent object
/// @param name String shown when dropdown is not open
/// @param options Options string (newline-separated)
/// @param selected Initially selected option index
/// @return Created dropdown object
lv_obj_t* CreateDropdown(lv_obj_t* parent, const char* name, const char* options, int32_t selected);

/// Create a styled button matrix (on-screen keyboard)
/// @param parent Parent object
/// @param btn_map Button map array (NULL-terminated)
/// @param event_cb Event callback for button presses
/// @return Created button matrix object
lv_obj_t* CreateButtonMatrix(lv_obj_t* parent, const char* btn_map[], lv_event_cb_t event_cb,
                             void* user_data = nullptr);

// ============================================================================
// CHART FACTORIES
// ============================================================================

/// Create a styled line chart
/// @param parent Parent object
/// @param points_capacity Maximum number of data points
/// @return Created chart object
lv_obj_t* CreateChart(lv_obj_t* parent, uint16_t points_capacity);

/// Create a styled chart scale
/// @param parent Parent object
/// @param tick_count Number of ticks on the scale
/// @param left If true, scale is left-aligned; otherwise right-aligned
/// @return Created chart scale object
lv_obj_t* CreateChartScale(lv_obj_t* parent, size_t tick_count, bool left);

// ============================================================================
// INDICATOR FACTORIES
// ============================================================================

/// Create an LED indicator
/// @param parent Parent object
/// @param initial_state Initial ON/OFF state
/// @return Created LED object
lv_obj_t* CreateLEDIndicator(lv_obj_t* parent, bool initial_state = false);

lv_obj_t* CreateMessageBox(lv_obj_t* parent, std::string title, std::string message = "",
                           std::string confirm_text = "OK", std::string cancel_text = "Cancel",
                           lv_event_cb_t on_confirm = nullptr, lv_event_cb_t on_cancel = nullptr,
                           void* user_data = nullptr);

lv_obj_t* CreateLabeledUnit(lv_obj_t* parent, const char* label_text, float value, const char* value_format,
                            const char* unit_text, bool small = false);

lv_obj_t* CreateLabeledUnit(lv_obj_t* parent, const char* label_text, const char* unit_text, lv_subject_t* subject,
                            const char* value_format, bool small = false);

lv_obj_t* CreateLabeledIntUnit(lv_obj_t* parent, const char* label_text, const char* unit_text, lv_subject_t* subject,
                               const char* value_format, bool small);

// ============================================================================
// HELPER FUNCTIONS
// ============================================================================

void ValueFloatObserverHandler(lv_observer_t* observer, lv_subject_t* subject);

/// Update a value label
/// @param label Label object created by CreateValue* functions
/// @param value New value
/// @param format Printf-style format string
void UpdateValueLabel(lv_obj_t* label, float value, const char* format = "%.0f");

/// Set LED indicator state
/// @param led LED object created by CreateLEDIndicator
/// @param on True for ON, false for OFF
void SetLEDState(lv_obj_t* led, bool on);

/// Convert a snake_case string to Title Case
/// @param snake_case Input snake_case string
/// @return Converted Title Case string
std::string SnakeToTitle(const std::string& snake_case);

/// Convert a Title Case string to snake_case
/// @param title_case Input Title Case string
/// @return Converted snake_case string
std::string TitleToSnake(const std::string& title_case);

}  // namespace toothless::ui
