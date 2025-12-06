#include "ui/screens/screen_helpers.hpp"

#include <lvgl.h>

#include "config.h"
#include "heater/heater.hpp"
#include "screen_helpers.hpp"
#include "ui/display/display.hpp"
#include "ui/screens/overlay_manager.hpp"
#include "ui/screenshot.hpp"
#include "ui/subjects.hpp"
#include "ui/themes/widget_factories.hpp"

namespace toothless {

lv_obj_t* MainChart(lv_obj_t* parent, size_t max_points) {
  lv_obj_t* wrapper = ui::CreateRowContainer(parent);
  lv_obj_set_size(wrapper, lv_pct(100), 0);
  // lv_obj_set_style_min_height(wrapper, 120, 0);
  lv_obj_set_flex_grow(wrapper, 1);

  lv_obj_t* chart = ui::CreateChart(wrapper, max_points);
  if (!chart) {
    FLOG_ERROR("Failed to create chart");
    return nullptr;
  }
  lv_obj_set_size(chart, 0, lv_pct(100));
  lv_obj_set_flex_grow(chart, 1);

  // lv_chart_set_update_mode(chart, LV_CHART_UPDATE_MODE_SHIFT);
  // lv_chart_set_update_mode(chart, LV_CHART_UPDATE_MODE_CIRCULAR);
  lv_chart_series_t* temp_series = lv_chart_add_series(chart, lv_palette_main(LV_PALETTE_RED), LV_CHART_AXIS_PRIMARY_Y);
  lv_chart_series_t* target_series =
      lv_chart_add_series(chart, lv_palette_main(LV_PALETTE_BLUE), LV_CHART_AXIS_PRIMARY_Y);

  // lv_chart_set_div_line_count(chart, kYLabelCount, 5);

  return chart;
}

void NumpadOpen(const NumpadContext& ctx) {
  FLOG_INFO("Open Numpad");
  auto& mgr = OverlayManager::Instance();
  if (mgr.IsActive()) return;

  auto* state = mgr.Open<NumpadState>(ctx.parent_screen);
  state->on_confirm = ctx.on_confirm;

  // clang-format off
  static const char *btn_map[] = {
      "1", "2", "3", "\n",
      "4", "5", "6", "\n",
      "7", "8", "9", "\n",
      LV_SYMBOL_STOP, "0", LV_SYMBOL_BACKSPACE, "\n",
      LV_SYMBOL_SAVE, LV_SYMBOL_CLOSE, NULL
  };
  // clang-format on

  state->numpadtextarea = lv_textarea_create(state->backdrop);

  lv_obj_remove_style_all(state->numpadtextarea);
  lv_textarea_set_one_line(state->numpadtextarea, true);
  lv_textarea_set_align(state->numpadtextarea, LV_TEXT_ALIGN_CENTER);
  lv_obj_set_flex_align(state->numpadtextarea, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  // lv_obj_set_scrollbar_mode(wrapper, LV_SCROLLBAR_MODE_OFF);
  // lv_obj_remove_flag(wrapper, LV_OBJ_FLAG_SCROLLABLE);
  lv_textarea_set_max_length(state->numpadtextarea, 3);
  lv_textarea_set_accepted_chars(state->numpadtextarea, "0123456789");
  if (ctx.initial_value.has_value()) {
    lv_textarea_set_text(state->numpadtextarea, std::to_string(ctx.initial_value.value()).c_str());
  } else {
    lv_textarea_set_text(state->numpadtextarea, "");
  }
  lv_textarea_set_text(state->numpadtextarea, "");
  lv_textarea_set_placeholder_text(state->numpadtextarea, "Enter value");
  lv_obj_set_size(state->numpadtextarea, lv_pct(100), lv_pct(15));
  lv_obj_set_style_pad_all(state->numpadtextarea, 0, 0);

  state->numpad = lv_buttonmatrix_create(state->backdrop);
  lv_obj_set_style_pad_all(state->numpad, 0, 0);
  lv_obj_set_style_bg_color(state->numpad, lv_color_black(), 0);
  lv_obj_set_style_border_width(state->numpad, 0, 0);
  lv_buttonmatrix_set_map(state->numpad, btn_map);
  lv_obj_add_event_cb(state->numpad, NumpadKeyHandler, LV_EVENT_VALUE_CHANGED, state);

  lv_obj_set_size(state->numpad, lv_pct(100), lv_pct(80));

  lv_obj_set_layout(state->numpad, LV_LAYOUT_GRID);
}

void NumpadKeyHandler(lv_event_t* e) {
  FLOG_INFO("Key Pressed");
  NumpadState* state = (NumpadState*)lv_event_get_user_data(e);

  lv_event_code_t code = lv_event_get_code(e);
  lv_obj_t* obj = lv_event_get_target_obj(e);

  if (code == LV_EVENT_VALUE_CHANGED) {
    uint32_t id = lv_buttonmatrix_get_selected_button(obj);
    const char* txt = lv_buttonmatrix_get_button_text(obj, id);
    FLOG_INFO("Key %s pressed", txt);
    LV_UNUSED(txt);
    LV_LOG_USER("%s was pressed\n", txt);
    if (!obj) return;
    if (lv_strcmp(txt, LV_SYMBOL_SAVE) == 0) {
      NumPadCleanupHandler(e);
    } else if (lv_strcmp(txt, LV_SYMBOL_BACKSPACE) == 0) {
      // Handle backspace
      lv_textarea_delete_char(state->numpadtextarea);
    } else if (lv_strcmp(txt, LV_SYMBOL_CLOSE) == 0) {
      OverlayManager::Instance().Close();
    } else if (lv_strcmp(txt, LV_SYMBOL_STOP) == 0) {
    } else {
      // Append character to textarea
      lv_textarea_add_text(state->numpadtextarea, txt);
    }
  }
}

void NumPadCleanupHandler(lv_event_t* e) {
  FLOG_INFO("cleanup started");
  NumpadState* state = (NumpadState*)lv_event_get_user_data(e);

  if (!state) {
    FLOG_ERROR("no state passed... propably have a memory leak...");
    return;
  }
  // On apply, read value and close keyboard
  const char* txt = lv_textarea_get_text(state->numpadtextarea);
  FLOG_INFO("Got Text: %s", txt);
  if (!txt) return;
  std::optional<int32_t> optval = std::nullopt;
  if (lv_strcmp(txt, "") != 0) {
    uint16_t val = atoi(txt);
    optval = val;
  }
  if (state->on_confirm) {
    FLOG_INFO("Running Callback");
    state->on_confirm(optval);
    FLOG_INFO("Done");
  }

  OverlayManager::Instance().Close();

  FLOG_INFO("All done");
}

void TimeRollerOpen(const NumberRollerContext& ctx) {
  FLOG_INFO("Open Time Roller");
  auto& mgr = OverlayManager::Instance();
  if (mgr.IsActive()) return;

  const char* minsecstr = "00\n05\n10\n15\n20\n25\n30\n35\n40\n45\n50\n55";

  auto* state = mgr.Open<NumberRollerState>(ctx.parent_screen);
  state->on_confirm = ctx.on_confirm;

  lv_obj_t* col;
  col = ui::CreateRowContainer(state->backdrop);
  lv_obj_set_flex_align(col, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  lv_obj_set_size(col, lv_pct(100), 0);
  lv_obj_set_flex_grow(col, 1);

  state->col_a = ui::CreateRoller(col, minsecstr, 00);
  lv_obj_add_event_cb(state->col_a, TimeRollerHandler, LV_EVENT_ALL, state);
  ui::CreateUnitLabel(col, ":");
  state->col_b = ui::CreateRoller(col, minsecstr, 00);
  lv_obj_add_event_cb(state->col_b, TimeRollerHandler, LV_EVENT_ALL, state);
  ui::CreateUnitLabel(col, ":");
  state->col_c = ui::CreateRoller(col, minsecstr, 00);
  lv_obj_add_event_cb(state->col_c, TimeRollerHandler, LV_EVENT_ALL, state);

  col = ui::CreateRowContainer(state->backdrop);
  lv_obj_set_width(col, lv_pct(100));
  lv_obj_set_height(col, LV_SIZE_CONTENT);
  lv_obj_set_flex_grow(col, 0);

  lv_obj_t* ok_btn = ui::CreatePrimaryButton(col, "Set Time", lv_pct(100), NULL, true);
  lv_obj_add_event_cb(ok_btn, TimeRollerCleanupHandler, LV_EVENT_CLICKED, state);
}

void TimeRollerHandler(lv_event_t* e) {
  // FLOG_INFO("Time Roller event");
  lv_event_code_t code = lv_event_get_code(e);
  lv_obj_t* obj = lv_event_get_target_obj(e);
  if (code == LV_EVENT_VALUE_CHANGED) {
    char buf[32];
    lv_roller_get_selected_str(obj, buf, sizeof(buf));
    FLOG_INFO("Selected: %s\n", buf);
  }
}

// TODO: make this a INTEGER roller ...., and make the total_seconds math configurable by passing context
void TimeRollerCleanupHandler(lv_event_t* e) {
  FLOG_INFO("Time Roller cleanup started");
  NumberRollerState* state = (NumberRollerState*)lv_event_get_user_data(e);
  if (!state) {
    FLOG_ERROR("no state passed...");
    return;
  }

  char buf[32];
  lv_roller_get_selected_str(state->col_a, buf, sizeof(buf));
  int hours = atoi(buf);
  lv_roller_get_selected_str(state->col_b, buf, sizeof(buf));
  int minutes = atoi(buf);
  lv_roller_get_selected_str(state->col_c, buf, sizeof(buf));
  int seconds = atoi(buf);
  FLOG_INFO("Got Time: %02d:%02d:%02d", hours, minutes, seconds);
  int32_t total_seconds = (hours * 3600 + minutes * 60 + seconds);

  if (state->on_confirm) {
    state->on_confirm(total_seconds);
  }
  OverlayManager::Instance().Close();
  FLOG_INFO("All done");
}

void TextAreaEventHandler(lv_event_t* e) {
  TextAreaFullscreenEventHandler(e);
  return;  // TODO: Figure out how to keep the text area above the keyboard.
  if (lv_display_get_vertical_resolution(NULL) <= 280) {
    TextAreaFullscreenEventHandler(e);
    return;
  }
  // ProfilesScreen* screen = (ProfilesScreen*)lv_event_get_user_data(e);
  lv_obj_t* parent = static_cast<lv_obj_t*>(lv_event_get_user_data(e));
  lv_obj_t* textarea = static_cast<lv_obj_t*>(lv_event_get_target(e));
  lv_event_code_t code = lv_event_get_code(e);
  static lv_obj_t* keyboard = nullptr;

  if (code == LV_EVENT_FOCUSED) {
    // Create keyboard if it doesn't exist
    if (!keyboard) {
      keyboard = lv_keyboard_create(lv_screen_active());
      lv_obj_set_size(keyboard, lv_pct(100), lv_pct(60));

      // Position at bottom using align
      lv_obj_align(keyboard, LV_ALIGN_BOTTOM_MID, 0, 0);

      // Make it floating (ignores parent layout)
      lv_obj_add_flag(keyboard, LV_OBJ_FLAG_FLOATING);
    }
    lv_keyboard_set_textarea(keyboard, textarea);
    lv_obj_remove_flag(keyboard, LV_OBJ_FLAG_HIDDEN);
    lv_obj_move_foreground(keyboard);
  } else if (code == LV_EVENT_DEFOCUSED) {
    if (keyboard) {
      lv_obj_add_flag(keyboard, LV_OBJ_FLAG_HIDDEN);
    }
  } else if (code == LV_EVENT_READY) {
    // User pressed "OK" button on keyboard
    FLOG_INFO("Keyboard OK pressed, text: %s", lv_textarea_get_text(textarea));
    // Hide keyboard
    if (keyboard) {
      lv_obj_add_flag(keyboard, LV_OBJ_FLAG_HIDDEN);
    }
    // lv_obj_send_event(textarea, LV_EVENT_READY, NULL);
  }
}

void TextAreaFullscreenEventHandler(lv_event_t* e) {
  lv_event_code_t code = lv_event_get_code(e);
  if (code != LV_EVENT_FOCUSED) return;

  auto& mgr = OverlayManager::Instance();
  if (mgr.IsActive()) return;

  lv_obj_t* parent = static_cast<lv_obj_t*>(lv_event_get_user_data(e));
  lv_obj_t* textarea = static_cast<lv_obj_t*>(lv_event_get_target(e));

  FLOG_INFO("Textarea focused - show fullscreen keyboard");

  auto* state = mgr.Open<TextAreaOverlayState>(parent);
  state->original_textarea = textarea;

  lv_obj_t* wrapper = ui::CreateColumnContainer(state->backdrop);
  lv_obj_set_size(wrapper, lv_pct(100), lv_pct(100));

  state->holder = ui::CreateTextArea(wrapper);
  lv_textarea_set_text(state->holder, lv_textarea_get_text(textarea));
  lv_textarea_set_cursor_pos(state->holder, lv_textarea_get_cursor_pos(textarea));
  lv_obj_set_size(state->holder, lv_pct(100), LV_SIZE_CONTENT);

  lv_obj_t* keyboard = lv_keyboard_create(wrapper);
  lv_obj_set_size(keyboard, lv_pct(100), lv_pct(60));
  lv_keyboard_set_textarea(keyboard, state->holder);

  // On keyboard OK - copy text back and close
  lv_obj_add_event_cb(
      keyboard,
      [](lv_event_t* e) {
        auto* state = static_cast<TextAreaOverlayState*>(lv_event_get_user_data(e));
        lv_textarea_set_text(state->original_textarea, lv_textarea_get_text(state->holder));
        lv_textarea_set_cursor_pos(state->original_textarea, lv_textarea_get_cursor_pos(state->holder));
        lv_obj_send_event(state->original_textarea, LV_EVENT_READY, NULL);
        OverlayManager::Instance().Close();
      },
      LV_EVENT_READY, state);

  // On keyboard Cancel - just close
  lv_obj_add_event_cb(keyboard, [](lv_event_t* e) { OverlayManager::Instance().Close(); }, LV_EVENT_CANCEL, nullptr);
}

void ConfirmationPopup(const ConfirmationContext& ctx) {
  FLOG_INFO("Open Confirmation");
  auto& mgr = OverlayManager::Instance();
  if (mgr.IsActive()) return;

  auto* state = mgr.Open<ConfirmationState>(ctx.parent_screen);
  state->object = ctx.object;
  state->cancel_text = ctx.cancel_text;
  state->confirm_text = ctx.confirm_text;
  state->title = ctx.title;
  state->message = ctx.message;
  state->on_cancel = ctx.on_cancel;
  state->on_confirm = ctx.on_confirm;

  lv_obj_t* msgbox = ui::CreateMessageBox(state->backdrop, state->title, state->message, state->confirm_text,
                                          state->cancel_text, ConfirmationHandler, ConfirmationHandler, state);
}

void ConfirmationHandler(lv_event_t* e) {
  ConfirmationState* state = (ConfirmationState*)lv_event_get_user_data(e);
  lv_obj_t* button = (lv_obj_t*)lv_event_get_target(e);
  lv_obj_t* label = lv_obj_get_child(button, 0);
  const char* text = lv_label_get_text(label);

  if (strcmp(text, state->confirm_text.c_str()) == 0) {
    FLOG_DEBUG("Confirmed!");
    if (state->on_confirm) state->on_confirm(state);
  } else if (strcmp(text, state->cancel_text.c_str()) == 0) {
    FLOG_DEBUG("Canceled");
    if (state->on_cancel) state->on_cancel(state);
  }
  OverlayManager::Instance().Close();
}

lv_obj_t* CreateModeSwitcher(lv_obj_t* screen) {
  auto& mgr = OverlayManager::Instance();
  if (mgr.IsActive()) return nullptr;

  auto* state = mgr.Open<ModeSwitcherState>(screen);

  lv_obj_t* wrapper = ui::CreateColumnContainer(state->backdrop);
  lv_obj_set_size(wrapper, lv_pct(100), lv_pct(100));
  lv_obj_set_style_pad_gap(wrapper, 10, 0);

  ui::CreateTitle(wrapper, "Select Mode");

  auto add_btn = [&](const char* txt) {
    lv_obj_t* btn = ui::CreatePrimaryButton(wrapper, txt, lv_pct(100), LV_SIZE_CONTENT, true);
    lv_obj_add_event_cb(btn, ModeSwitcherHandler, LV_EVENT_CLICKED, nullptr);
  };
  add_btn("Select Profile");
  add_btn("Drying");
  add_btn("Reflow");
  add_btn("Cancel");
  add_btn("Snapshot");

  return wrapper;
}

void ModeSwitcherHandler(lv_event_t* e) {
  FLOG_INFO("Event Handler Called");
  lv_obj_t* button = (lv_obj_t*)lv_event_get_target(e);
  lv_obj_t* label = lv_obj_get_child_by_type(button, 0, &lv_label_class);
  if (!label) return;
  const char* text = lv_label_get_text(label);

  if (lv_strcmp(text, "Drying") == 0) {
    PS_PUB_INT(topics::heater::mode_set, heater::Mode::kModeDrying);
  } else if (lv_strcmp(text, "Reflow") == 0) {
    PS_PUB_INT(topics::heater::mode_set, heater::Mode::kModeReflow);
  } else if (lv_strcmp(text, "Select Profile") == 0) {
    PS_PUB_NIL("ui.action.profiles");
  } else if (lv_strcmp(text, "Snapshot") == 0) {
    FLOG_INFO("Taking Screenshot");
    TakeSnapshot();
  }
  OverlayManager::Instance().Close();
}

lv_obj_t* CreateMidSection(lv_obj_t* parent) {
  std::shared_ptr<Subjects> subjects = SubjectManager::Instance().subjects;
  lv_obj_t* mid = ui::CreateRowContainer(parent);
  lv_obj_set_height(mid, LV_SIZE_CONTENT);
  lv_obj_set_width(mid, lv_pct(100));
  // lv_obj_set_flex_grow(mid, 1);  //<<<<<<<<<<<<<<<<<<<<<<<
  lv_obj_set_flex_flow(mid, LV_FLEX_FLOW_ROW_WRAP);
  lv_obj_set_flex_align(mid, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  // lv_obj_set_style_pad_gap(mid, 5, 0);  // Gap between blocks

  // lv_obj_set_style_flex_cross_place(mid, LV_FLEX_ALIGN_CENTER, 0);
  // lv_obj_set_style_flex_main_place(mid, LV_FLEX_ALIGN_CENTER, 0);

  {
    lv_obj_t* profile = ui::CreateContainer(mid);
    lv_obj_set_size(profile, LV_SIZE_CONTENT, LV_SIZE_CONTENT);

    lv_obj_t* profile_label = ui::CreateBodyText(profile, "No Profile Loaded");
    lv_obj_set_size(profile_label, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_label_bind_text(profile_label, &subjects->profile, "%s");
    lv_obj_add_flag(profile, LV_OBJ_FLAG_HIDDEN);
    lv_obj_bind_flag_if_eq(profile, &subjects->show_profile, LV_OBJ_FLAG_HIDDEN, 0);
  }
  {
    lv_obj_t* stage = ui::CreateContainer(mid);
    ui::CreateSmallText(stage, LV_SYMBOL_RIGHT);
    lv_obj_t* stage_label = ui::CreateBodyText(stage, "-");
    lv_label_bind_text(stage_label, &subjects->stage, "%s");
    lv_obj_add_flag(stage, LV_OBJ_FLAG_HIDDEN);
    lv_obj_bind_flag_if_eq(stage, &subjects->show_stage, LV_OBJ_FLAG_HIDDEN, 0);
  }

  return mid;
}

lv_obj_t* CreateBottomRow(lv_obj_t* container) {
  static lv_obj_t* start_stop;
  lv_obj_t* wrapper = ui::CreateRowContainer(container);
  lv_obj_set_size(wrapper, lv_pct(100), LV_SIZE_CONTENT);
  lv_obj_set_style_pad_gap(wrapper, 10, 0);

  start_stop = CreateStartStopButton(wrapper);
  CreateModeButton(wrapper);
  CreateSettingsButton(wrapper);
  return start_stop;
}

lv_obj_t* CreateStartStopButton(lv_obj_t* container) {
  std::shared_ptr<Subjects> subjects = SubjectManager::Instance().subjects;

  lv_obj_t* button = ui::CreatePrimaryButton(container, "Start", LV_SIZE_CONTENT, NULL, true);
  lv_obj_t* startstop_label = lv_obj_get_child_by_type(button, 0, &lv_label_class);
  lv_obj_add_event_cb(button, ButtonEventHandler, LV_EVENT_CLICKED, startstop_label);
  lv_label_bind_text(startstop_label, &subjects->start_stop, "%s");
  return startstop_label;
}

lv_obj_t* CreateModeButton(lv_obj_t* container) {
  lv_obj_t* button = ui::CreatePrimaryButton(container, "Mode", LV_SIZE_CONTENT, NULL, true);
  lv_obj_add_event_cb(button, ButtonEventHandler, LV_EVENT_CLICKED, nullptr);
  return button;
}

lv_obj_t* CreateSettingsButton(lv_obj_t* container) {
  lv_obj_t* button = ui::CreateSettingsButton(container, LV_SIZE_CONTENT, NULL, false);
  lv_obj_add_event_cb(button, ButtonEventHandler, LV_EVENT_CLICKED, NULL);
  return button;
}

lv_obj_t* CreateCBButton(lv_obj_t* parent, const char* txt, bool grow, lv_event_cb_t callback, void* user_data) {
  lv_obj_t* btn = ui::CreatePrimaryButton(parent, txt, grow ? lv_pct(100) : LV_SIZE_CONTENT,
                                          grow ? 0 : lv_display_get_vertical_resolution(NULL) * 0.2, grow);
  lv_obj_add_event_cb(btn, callback, LV_EVENT_CLICKED, user_data);

  return btn;
}

void ButtonEventHandler(lv_event_t* e) {
  FLOG_INFO("Event Handler Called");
  lv_obj_t* button = (lv_obj_t*)lv_event_get_target(e);
  lv_obj_t* screen = (lv_obj_t*)lv_event_get_user_data(e);

  // lv_obj_t* label = lv_obj_get_child(button, 0);  // Button's label
  lv_obj_t* label = lv_obj_get_child_by_type(button, 0, &lv_label_class);
  if (!label) return;
  const char* text = lv_label_get_text(label);

  if (lv_strcmp(text, "Start") == 0) {
    FLOG_DEBUG("Start Button clicked!");
    PS_PUB_NIL(topics::heater::start);
    // lv_label_set_text(label, "Stop");
    // PS_PUB_NIL("ui.action.start");
    return;
  } else if (lv_strcmp(text, "Stop") == 0) {
    CreateStopConfirmation(label);
    // lv_label_set_text(label, "Start");
    // Get the screen object if you passed it as user_data
    // ReflowScreen* screen = (ReflowScreen*)lv_event_get_user_data(e);
    // if (screen) {
    //   screen->StopConfirmation();
    // }
    // TODO: figure out stop confirmation
  } else if (lv_strcmp(text, "Mode") == 0) {
    if (screen) {
      CreateModeSwitcher(screen);
    } else {
      CreateModeSwitcher(lv_display_get_screen_active(NULL));
    }
  } else if (lv_strcmp(text, LV_SYMBOL_SETTINGS) == 0) {
    FLOG_DEBUG("Settings Button clicked!");
    PS_PUB_NIL("ui.action.settings");
  } else {
    FLOG_DEBUG("Option unknown: %s", text);
  }

  // lv_event_code_t code = lv_event_get_code(e);
  // if (code == LV_EVENT_CLICKED) {}
}

void CreateStopConfirmation(lv_obj_t* label) {
  lv_obj_t* screen = lv_display_get_screen_active(NULL);

  ConfirmationContext ctx{.parent_screen = screen,
                          .backdrop = nullptr,
                          .object = label,
                          .title = "Stop?",
                          .message = "Are you sure you want to stop?",
                          .confirm_text = "Stop!",
                          .cancel_text = "Cancel",
                          .on_confirm =
                              [](void* obj) {
                                FLOG_INFO("Stopping");
                                PS_PUB_NIL(topics::heater::stop);
                                // mgr.Close() called by ConfirmationHandler
                              },
                          .on_cancel =
                              [](void* obj) {
                                FLOG_INFO("Cancelled");
                                // mgr.Close() called by ConfirmationHandler
                              }};
  ConfirmationPopup(ctx);
}

void StopConfirmationHandler(lv_event_t* e) {
  lv_obj_t* button = (lv_obj_t*)lv_event_get_target(e);
  lv_obj_t* backdrop = (lv_obj_t*)lv_event_get_user_data(e);

  // Get button text to determine action
  lv_obj_t* label = lv_obj_get_child(button, 0);  // Button's label
  const char* text = lv_label_get_text(label);

  if (lv_strcmp(text, "Stop!") == 0) {
    FLOG_DEBUG("Stop confirmed!");
    PS_PUB_NIL(topics::heater::stop);

    // PS_PUB_NIL("ui.action.stop");
    // return;
  } else if (lv_strcmp(text, "Cancel") == 0) {
  } else if (lv_strcmp(text, LV_SYMBOL_SETTINGS) == 0) {
    PS_PUB_NIL("ui.action.settings");
  }
  OverlayManager::Instance().Close();
}

void AutoDeleter(lv_obj_t* target) {
  lv_obj_add_event_cb(
      target,
      [](lv_event_t* e) {
        lv_obj_t* obj = static_cast<lv_obj_t*>(lv_event_get_target(e));
        FLOG_DEBUG("Auto deleting object %p", obj);
        lv_obj_delete(obj);
      },
      LV_EVENT_DELETE, NULL);
}
}  // namespace toothless