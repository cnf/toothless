# STATUS.md - LVGL Screen Management Analysis
Date: 2025-11-01 23:21

## Instructions
- Do NOT edit files unless specifically told to

## Context
User is building ESP32 reflow oven controller with:
- LVGL display (ST7796S, 320x480, touch XPT2046)
- pubsub-c message bus for data transfer
- Current Display class has basic LVGL init with single label

## Current Structure
- Display class in main/display/ handles LVGL init, mutex, task
- Pubsub subscriptions: "sensor.chamber.temperature", "heater.status"
- Heater class publishes temp data, PID control
- Main components: Display, Heater, Sensors (MAX6675)

## Request
User wants suggestions for LVGL screen management structure options.

## Analysis Status
- [x] Reviewed current Display implementation
- [x] Identified pubsub message patterns
- [x] Located heater/sensor data flow
- [x] Provide screen management options
- [x] User asking about namespace pattern validation

## Notes
- User is implementing screen_manager.cpp
- All code wrapped in `toothless` namespace
- Question about namespace pattern for collision avoidance
- User chose Option 1: Screen Manager Class
- Currently working on screen_manager.hpp
- USER PREFERENCE: Suggestions only, no code implementation unless specifically requested
- Current question: Memory management and object ownership options
- User reverted to raw pointers lv_obj_t*
- AGREED: Option 3 (hybrid ownership) - track screen containers only
- User considering lazy creation + create/destroy on switch for memory optimization
- CONTEXT: ESP32 dual core - reflow logic on core 1, UI on core 0
- AGREED: Create/destroy on switch is viable with dual-core
- User asking about GetXXXScreen() pattern vs map storage for lazy creation
- DECISION: Option A (Pure Factory Methods) - always create new, can evolve to Option B later
- User commented out map, converted Create methods to Get methods
- CURRENT FOCUS: Single screen first, learn LVGL basics - elements and data updates
- Starting with temperature display label on HOME screen
- Now working on home_screen.hpp - focusing on screen creation first
- Future requirement: Support different screen shapes/UI forms
- DECISION: Use screen classes (no BaseScreen yet), but unclear on UpdateXXX pattern
- User suggesting each screen class has Update()/Loop() method and handles own pubsub updates
- AGREED: Each screen handles own pubsub subscriptions and widget updates
- User asking about putting cleanup logic in HomeScreen destructor
- AGREED: HomeScreen destructor only cleans pubsub, ScreenManager handles LVGL objects
- User implemented SwitchTo() - checking if it aligns with ownership model
- User refactored to use BaseScreen abstract class with proper C++ object tracking
- USER PREFERENCE REMINDER: Suggestions only, no code implementation unless specifically requested
- User refactored UI code structure, created UserInterface class but not satisfied with it
- User identified Display class becomes passive after init, suggests static pattern
- User considering merging ScreenManager + UserInterface into single class
- User implemented static Display class and merged ScreenManager into UserInterface
- Architecture complete, ready to implement first HomeScreen with LVGL widgets
- User experiencing crash in Display::Init() line 31 during xTaskCreatePinnedToCore call
- Task starts successfully but crashes in lvgl_port_task after checking for active screen (log C)