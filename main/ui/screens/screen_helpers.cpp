#include "ui/screens/screen_helpers.hpp"

#include <lvgl.h>

#include "config.h"
#include "heater/heater.hpp"
namespace toothless {

lv_obj_t* CreateBackdrop(lv_obj_t* screen) {
  lv_obj_t* backdrop = lv_obj_create(screen);
  lv_obj_set_size(backdrop, lv_pct(100), lv_pct(100));
  lv_obj_set_pos(backdrop, 0, 0);
  lv_obj_remove_flag(backdrop, LV_OBJ_FLAG_FLEX_IN_NEW_TRACK);
  lv_obj_add_flag(backdrop, LV_OBJ_FLAG_FLOATING);
  lv_obj_set_layout(backdrop, LV_LAYOUT_FLEX);
  lv_obj_set_flex_flow(backdrop, LV_FLEX_FLOW_COLUMN);
  lv_obj_move_to_index(backdrop, -1);
  lv_obj_set_scrollbar_mode(backdrop, LV_SCROLLBAR_MODE_OFF);
  lv_obj_remove_flag(backdrop, LV_OBJ_FLAG_SCROLLABLE);

  return backdrop;
}

void NumpadOpen(const NumpadContext& ctx) {
  FLOG_INFO("Open Numpad");

  // create a small textarea + keyboard
  if (ctx.backdrop) return;  // already a backdrop active

  // NumpadState *state = new (NumpadState);
  NumpadState* state = new NumpadState();
  state->backdrop = ctx.backdrop;
  state->target_spinbox = ctx.target_spinbox;
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

  state->backdrop = CreateBackdrop(ctx.parent_screen);

  state->numpadtextarea = lv_textarea_create(state->backdrop);
  lv_textarea_set_one_line(state->numpadtextarea, true);
  lv_textarea_set_align(state->numpadtextarea, LV_TEXT_ALIGN_CENTER);
  lv_textarea_set_max_length(state->numpadtextarea, 3);
  lv_textarea_set_accepted_chars(state->numpadtextarea, "0123456789");
  lv_textarea_set_text(state->numpadtextarea, "");
  lv_textarea_set_placeholder_text(state->numpadtextarea, "Enter value");
  lv_obj_set_size(state->numpadtextarea, lv_pct(100), lv_pct(15));
  // lv_obj_align(state->numpadtextarea, LV_ALIGN_TOP_MID, 0, 0);
  lv_obj_set_flex_align(state->numpadtextarea, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  // lv_obj_set_pos(state->numpadtextarea, 10, 10);
  // lv_obj_set_size(state->numpadtextarea, 300, 50);                             // explicit px
  // lv_obj_set_style_bg_color(state->numpadtextarea, lv_color_hex(0xFF0000), 0); // red

  state->numpad = lv_buttonmatrix_create(state->backdrop);
  lv_buttonmatrix_set_map(state->numpad, btn_map);
  lv_obj_add_event_cb(state->numpad, NumpadKeyHandler, LV_EVENT_VALUE_CHANGED, state);
  // lv_obj_align(state->numpad, LV_ALIGN_CENTER, 0, 0);
  // lv_obj_set_flex_align(state->numpad, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

  lv_obj_set_size(state->numpad, lv_pct(100), lv_pct(85));

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
      if (state->backdrop) lv_obj_delete(state->backdrop);
    } else if (lv_strcmp(txt, LV_SYMBOL_STOP) == 0) {
      // TODO: how do we set NAN for a spinbox?
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
    val = std::clamp<uint16_t>(val, static_cast<uint16_t>(0), static_cast<uint16_t>(kUIMaxTargetTemperatureC));

    if (state->target_spinbox != nullptr) {
      lv_spinbox_set_value(state->target_spinbox, val);  // TODO: should be updated by pubsub
      FLOG_INFO("Set spinbox to %d", val);
    }
    optval = val * 100;
  }
  if (state->on_confirm) {
    FLOG_INFO("Running Callback");
    state->on_confirm(optval);
    FLOG_INFO("Done");
  }

  if (state->backdrop) lv_obj_delete(state->backdrop);
  state->backdrop = nullptr;
  if (state->numpad) lv_obj_delete(state->numpad);
  state->numpad = nullptr;
  if (state->numpadtextarea) lv_obj_delete(state->numpadtextarea);
  if (state->ok_btn) lv_obj_delete(state->ok_btn);
  FLOG_INFO("All done");

  delete state;
}

void TimeRollerOpen(const TimeRollerContext& ctx) {
  FLOG_INFO("Open Time Roller");
  const char* minsecstr =
      "00\n"
      "05\n"
      "10\n"
      "15\n"
      "20\n"
      "25\n"
      "30\n"
      "35\n"
      "40\n"
      "45\n"
      "50\n"
      "55";

  // create a small textarea + keyboard
  if (ctx.backdrop) return;  // already a backdrop active

  TimeRollerState* state = new TimeRollerState();
  state->backdrop = ctx.backdrop;
  state->target_spinbox = ctx.target_spinbox;
  state->on_confirm = ctx.on_confirm;

  state->backdrop = CreateBackdrop(ctx.parent_screen);
  lv_obj_t* col;
  {
    col = lv_obj_create(state->backdrop);
    lv_obj_remove_style_all(col);
    lv_obj_set_style_bg_opa(col, LV_OPA_TRANSP, 0);
    lv_obj_set_size(col, lv_pct(100), LV_SIZE_CONTENT);
    lv_obj_set_layout(col, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(col, LV_FLEX_FLOW_ROW);  // Vertical stacking
    lv_obj_set_flex_align(col, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START);
    lv_obj_t* label = lv_label_create(col);
    lv_label_set_text(label, "Timer");
  }

  col = lv_obj_create(state->backdrop);
  lv_obj_remove_style_all(col);
  lv_obj_set_style_bg_opa(col, LV_OPA_TRANSP, 0);
  lv_obj_set_layout(col, LV_LAYOUT_FLEX);
  lv_obj_set_flex_flow(col, LV_FLEX_FLOW_ROW);  // Vertical stacking
  lv_obj_set_flex_align(col, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  // lv_obj_set_size(col, lv_pct(100), LV_SIZE_CONTENT);
  lv_obj_set_size(col, lv_pct(100), 0);
  lv_obj_set_flex_grow(col, 1);
  {
    // Hours roller
    state->roller_hours = lv_roller_create(col);
    lv_roller_set_options(state->roller_hours, minsecstr, LV_ROLLER_MODE_INFINITE);

    lv_roller_set_visible_row_count(state->roller_hours, 4);
    lv_obj_center(state->roller_hours);
    lv_obj_add_event_cb(state->roller_hours, TimeRollerHandler, LV_EVENT_ALL, state);
  }
  {
    // Minutes roller
    state->roller_minutes = lv_roller_create(col);
    lv_roller_set_options(state->roller_minutes, minsecstr, LV_ROLLER_MODE_INFINITE);

    lv_roller_set_visible_row_count(state->roller_minutes, 4);
    lv_obj_center(state->roller_minutes);
    lv_obj_add_event_cb(state->roller_minutes, TimeRollerHandler, LV_EVENT_ALL, state);
  }
  {
    // Seconds roller
    state->roller_seconds = lv_roller_create(col);
    lv_roller_set_options(state->roller_seconds, minsecstr, LV_ROLLER_MODE_INFINITE);

    lv_roller_set_visible_row_count(state->roller_seconds, 4);
    lv_obj_center(state->roller_seconds);
    lv_obj_add_event_cb(state->roller_seconds, TimeRollerHandler, LV_EVENT_ALL, state);
  }
  {
    col = lv_obj_create(state->backdrop);
    lv_obj_set_layout(col, LV_LAYOUT_FLEX);
    lv_obj_remove_style_all(col);
    lv_obj_set_style_bg_opa(col, LV_OPA_TRANSP, 0);
    // lv_obj_set_flex_flow(obj, LV_FLEX_FLOW_ROW);  // Vertical stacking
    // lv_obj_set_flex_align(obj, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_END);
    lv_obj_set_size(col, lv_pct(100), LV_SIZE_CONTENT);
    lv_obj_t* ok_btn = lv_button_create(state->backdrop);
    lv_obj_set_size(ok_btn, lv_pct(100), 60);
    lv_obj_t* ok_label = lv_label_create(ok_btn);
    lv_label_set_text(ok_label, "Set Time");
    lv_obj_center(ok_label);
    lv_obj_add_event_cb(ok_btn, TimeRollerCleanupHandler, LV_EVENT_CLICKED, state);
  }
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

void TimeRollerCleanupHandler(lv_event_t* e) {
  FLOG_INFO("Time Roller cleanup started");
  TimeRollerState* state = (TimeRollerState*)lv_event_get_user_data(e);

  if (!state) {
    FLOG_ERROR("no state passed... propably have a memory leak...");
    return;
  }
  // On apply, read value and close keyboard
  char buf[32];
  lv_roller_get_selected_str(state->roller_hours, buf, sizeof(buf));
  int hours = atoi(buf);
  lv_roller_get_selected_str(state->roller_minutes, buf, sizeof(buf));
  int minutes = atoi(buf);
  lv_roller_get_selected_str(state->roller_seconds, buf, sizeof(buf));
  int seconds = atoi(buf);
  FLOG_INFO("Got Time: %02d:%02d:%02d", hours, minutes, seconds);
  int32_t total_seconds = (hours * 3600 + minutes * 60 + seconds);  // TODO: units

  if (state->on_confirm) {
    FLOG_INFO("Running Callback");
    state->on_confirm(total_seconds);
    FLOG_INFO("Done");
  }

  if (state->backdrop) lv_obj_delete(state->backdrop);
  state->backdrop = nullptr;
  if (state->roller_hours) lv_obj_delete(state->roller_hours);
  state->roller_hours = nullptr;
  if (state->roller_minutes) lv_obj_delete(state->roller_minutes);
  state->roller_minutes = nullptr;
  if (state->roller_seconds) lv_obj_delete(state->roller_seconds);
  state->roller_seconds = nullptr;
  FLOG_INFO("All done");

  delete state;
}

void ConfirmationPopup(const ConfirmationContext& ctx) {
  FLOG_INFO("Open Confirmation");

  if (ctx.backdrop) return;  // already a backdrop active
  FLOG_INFO("Confirming...");

  ConfirmationState* state = new ConfirmationState();
  state->backdrop = ctx.backdrop;

  state->backdrop = CreateBackdrop(ctx.parent_screen);
  state->object = ctx.object;
  state->cancel_text = ctx.cancel_text;
  state->confirm_text = ctx.confirm_text;
  state->title = ctx.title;
  state->message = ctx.message;
  state->on_cancel = ctx.on_cancel;
  state->on_confirm = ctx.on_confirm;

  lv_obj_t* msgbox = lv_msgbox_create(state->backdrop);
  lv_obj_set_size(msgbox, lv_pct(100), lv_pct(60));

  // // Remove from flex layout so it can be positioned freely
  // lv_obj_remove_flag(msgbox, LV_OBJ_FLAG_FLEX_IN_NEW_TRACK);
  // lv_obj_add_flag(msgbox, LV_OBJ_FLAG_FLOATING);

  lv_obj_center(msgbox);
  lv_obj_move_to_index(msgbox, -1);  // Move to top of children
  lv_obj_set_style_text_align(msgbox, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);

  // FIXME: i think this will break...
  lv_msgbox_add_text(msgbox, ctx.message.c_str());
  lv_msgbox_add_title(msgbox, ctx.title.c_str());

  lv_obj_t* cancel_button = lv_msgbox_add_footer_button(msgbox, state->cancel_text.c_str());
  lv_obj_set_width(cancel_button, lv_pct(45));
  lv_obj_set_height(cancel_button, 100);
  // lv_obj_set_flex_grow(cancel_button, 1); // Chart grows to fill remaining space
  lv_obj_add_event_cb(cancel_button, ConfirmationHandler, LV_EVENT_CLICKED, state);

  lv_obj_t* confirm_button = lv_msgbox_add_footer_button(msgbox, state->confirm_text.c_str());
  lv_obj_set_width(confirm_button, lv_pct(45));
  lv_obj_set_height(confirm_button, 100);
  // lv_obj_set_flex_grow(confirm_button, 1); // Chart grows to fill remaining space
  lv_obj_add_event_cb(confirm_button, ConfirmationHandler, LV_EVENT_CLICKED, state);

  lv_obj_t* footer = lv_msgbox_get_footer(msgbox);
  lv_obj_set_height(footer, lv_pct(33));
}

void ConfirmationHandler(lv_event_t* e) {
  ConfirmationState* state = (ConfirmationState*)lv_event_get_user_data(e);
  lv_obj_t* button = (lv_obj_t*)lv_event_get_target(e);

  // Get button text to determine action
  lv_obj_t* label = lv_obj_get_child(button, 0);  // Button's label
  const char* text = lv_label_get_text(label);

  if (strcmp(text, state->confirm_text.c_str()) == 0) {
    FLOG_DEBUG("Confirmed!");
    if (state->on_confirm) state->on_confirm(state);
  } else if (strcmp(text, state->cancel_text.c_str()) == 0) {
    FLOG_DEBUG("Canceled");
    if (state->on_cancel) state->on_cancel(state);
  }
  if (state->backdrop) {
    lv_obj_delete_async(state->backdrop);
  }
}

lv_obj_t* CreateModeSwitcher(lv_obj_t* screen) {
  lv_obj_t* backdrop = CreateBackdrop(screen);

  lv_obj_t* wrapper = lv_obj_create(backdrop);
  lv_obj_remove_style_all(wrapper);
  // lv_obj_set_style_bg_opa(wrapper, LV_OPA_TRANSP, 0);
  lv_obj_set_style_pad_all(wrapper, 0, 0);
  lv_obj_set_size(wrapper, lv_pct(100), lv_pct(100));
  lv_obj_set_layout(wrapper, LV_LAYOUT_FLEX);
  lv_obj_set_flex_flow(wrapper, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(wrapper, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START);
  // lv_obj_set_scrollbar_mode(wrapper, LV_SCROLLBAR_MODE_OFF);
  // lv_obj_remove_flag(wrapper, LV_OBJ_FLAG_SCROLLABLE);
  // lv_obj_set_style_border_width(wrapper, 1, 0);
  lv_obj_set_style_bg_color(wrapper, lv_color_hex(0x440000), 0);

  lv_obj_set_style_pad_gap(wrapper, 10, 0);

  CreateText(wrapper, LV_SYMBOL_WARNING, "Switching mode will stop the current one.", false);

  CreateCBButton(wrapper, "Drying", true, ModeSwitcherHandler, backdrop);
  CreateCBButton(wrapper, "Reflow", true, ModeSwitcherHandler, backdrop);
  CreateCBButton(wrapper, "Cancel", true, ModeSwitcherHandler, backdrop);

  return wrapper;
}

void ModeSwitcherHandler(lv_event_t* e) {
  FLOG_INFO("Event Handler Called");
  lv_obj_t* button = (lv_obj_t*)lv_event_get_target(e);
  lv_obj_t* backdrop = (lv_obj_t*)lv_event_get_user_data(e);

  // lv_obj_t* label = lv_obj_get_child(button, 0);  // Button's label
  lv_obj_t* label = lv_obj_get_child_by_type(button, 0, &lv_label_class);
  if (!label) return;
  const char* text = lv_label_get_text(label);

  if (lv_strcmp(text, "Drying") == 0) {
    FLOG_INFO("Setting Drying Mode");
    PS_PUB_INT("heater.mode.set", heater::kModeDrying);
  } else if (lv_strcmp(text, "Reflow") == 0) {
    FLOG_INFO("Setting Reflow Mode");
    PS_PUB_INT("heater.mode.set", heater::kModeReflow);
  } else if (lv_strcmp(text, "Cancel") == 0) {
  }
  lv_obj_delete_async(backdrop);
}

lv_obj_t* CreateBottomRow(lv_obj_t* container) {
  static lv_obj_t* start_stop;
  lv_obj_t* wrapper = lv_obj_create(container);
  lv_obj_remove_style_all(wrapper);
  lv_obj_set_style_bg_opa(wrapper, LV_OPA_TRANSP, 0);
  lv_obj_set_style_pad_all(wrapper, 0, 0);
  lv_obj_set_scrollbar_mode(wrapper, LV_SCROLLBAR_MODE_OFF);
  lv_obj_remove_flag(wrapper, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_size(wrapper, lv_pct(100), 60);
  // lv_obj_set_size(wrapper, lv_pct(100), lv_pct(15));
  // lv_obj_set_style_min_height(wrapper, 60, 0);
  lv_obj_set_layout(wrapper, LV_LAYOUT_FLEX);
  lv_obj_set_flex_flow(wrapper, LV_FLEX_FLOW_ROW);
  lv_obj_set_style_pad_gap(wrapper, 10, 0);  // Gap between temp blocks

  start_stop = CreateStartStopButton(wrapper);
  CreateModeButton(wrapper);
  CreateSettingsButton(wrapper);
  return start_stop;
}

lv_obj_t* CreateStartStopButton(lv_obj_t* container) {
  lv_obj_t* button = lv_button_create(container);
  lv_obj_set_size(button, 0, lv_pct(100));
  lv_obj_set_flex_grow(button, 1);  // share space equally

  lv_obj_t* startstop_label = lv_label_create(button);
  lv_label_set_text(startstop_label, "Start");
  lv_obj_center(startstop_label);

  lv_obj_set_style_align(button, LV_ALIGN_BOTTOM_RIGHT, 0);  // or LV_ALIGN_RIGHT

  lv_obj_add_event_cb(button, ButtonEventHandler, LV_EVENT_CLICKED, startstop_label);

  return startstop_label;
}

lv_obj_t* CreateModeButton(lv_obj_t* container) {
  lv_obj_t* button = lv_button_create(container);
  lv_obj_set_size(button, 0, lv_pct(100));
  lv_obj_set_flex_grow(button, 1);

  lv_obj_t* startstop_label = lv_label_create(button);
  lv_label_set_text(startstop_label, "Mode");
  lv_obj_center(startstop_label);

  lv_obj_set_style_align(button, LV_ALIGN_BOTTOM_RIGHT, 0);  // or LV_ALIGN_RIGHT

  lv_obj_add_event_cb(button, ButtonEventHandler, LV_EVENT_CLICKED, startstop_label);

  return startstop_label;
}

lv_obj_t* CreateSettingsButton(lv_obj_t* container) {
  lv_obj_t* button = lv_button_create(container);
  lv_obj_set_size(button, LV_SIZE_CONTENT, lv_pct(100));
  // lv_obj_set_flex_grow(button, 1);  // share space equally

  lv_obj_t* label = lv_label_create(button);
  lv_label_set_text(label, LV_SYMBOL_SETTINGS);
  lv_obj_center(label);
  lv_obj_set_style_align(button, LV_ALIGN_BOTTOM_RIGHT, 0);  // or LV_ALIGN_RIGHT

  lv_obj_add_event_cb(button, ButtonEventHandler, LV_EVENT_CLICKED, NULL);

  return button;
}

lv_obj_t* CreateText(lv_obj_t* parent, const char* icon, const char* txt, bool builder_variant) {
  return CreateText(parent, icon, txt, NULL, builder_variant);
}

lv_obj_t* CreateText(lv_obj_t* parent, const char* icon, const char* txt, const char* fmt, bool builder_variant) {
  lv_obj_t* obj = lv_menu_cont_create(parent);

  lv_obj_t* img = NULL;
  lv_obj_t* label = NULL;

  if (icon) {
    img = lv_image_create(obj);
    lv_image_set_src(img, icon);
  }

  if (txt) {
    label = lv_label_create(obj);
    lv_label_set_text_fmt(label, txt, fmt);
    lv_label_set_long_mode(label, LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_obj_set_flex_grow(label, 1);
  }

  if (builder_variant && icon && txt) {
    lv_obj_add_flag(img, LV_OBJ_FLAG_FLEX_IN_NEW_TRACK);
    lv_obj_swap(img, label);
  }

  return obj;
}

lv_obj_t* CreateSwitch(lv_obj_t* parent, const char* icon, const char* txt, bool chk) {
  lv_obj_t* obj = CreateText(parent, icon, txt, false);

  lv_obj_t* sw = lv_switch_create(obj);
  lv_obj_add_state(sw, chk ? LV_STATE_CHECKED : LV_STATE_DEFAULT);

  return sw;
}

lv_obj_t* CreateSlider(lv_obj_t* parent, const char* icon, const char* txt, int32_t min, int32_t max, int32_t val) {
  lv_obj_t* obj = CreateText(parent, icon, txt, true);

  lv_obj_t* slider = lv_slider_create(obj);
  lv_obj_set_flex_grow(slider, 1);
  lv_slider_set_range(slider, min, max);
  lv_slider_set_value(slider, val, LV_ANIM_OFF);

  if (icon == NULL) {
    lv_obj_add_flag(slider, LV_OBJ_FLAG_FLEX_IN_NEW_TRACK);
  }

  return obj;
}

lv_obj_t* CreateButton(lv_obj_t* parent, const char* txt, bool grow) {
  lv_obj_t* btn = lv_button_create(parent);
  lv_obj_set_style_min_height(btn, 60, 0);
  lv_obj_set_style_min_width(btn, 60, 0);
  if (grow) {
    lv_obj_set_size(btn, lv_pct(100), 0);
    lv_obj_set_flex_grow(btn, 1);
  } else {
    lv_obj_set_size(btn, LV_SIZE_CONTENT, 60);
  }
  lv_obj_t* btn_label = lv_label_create(btn);
  if (txt) lv_label_set_text(btn_label, txt);
  lv_obj_center(btn_label);

  return btn;
}

lv_obj_t* CreateCBButton(lv_obj_t* parent, const char* txt, bool grow, lv_event_cb_t callback, void* user_data) {
  lv_obj_t* btn = CreateButton(parent, txt, grow);
  lv_obj_add_event_cb(btn, callback, LV_EVENT_CLICKED, user_data);

  return btn;
}

void ButtonEventHandler(lv_event_t* e) {
  FLOG_INFO("Event Handler Called");
  lv_obj_t* button = (lv_obj_t*)lv_event_get_target(e);

  // lv_obj_t* label = lv_obj_get_child(button, 0);  // Button's label
  lv_obj_t* label = lv_obj_get_child_by_type(button, 0, &lv_label_class);
  if (!label) return;
  const char* text = lv_label_get_text(label);

  if (lv_strcmp(text, "Start") == 0) {
    FLOG_DEBUG("Start Button clicked!");
    // PS_PUB_INT("heater.state.set", heater::kStateOn);
    PS_PUB_NIL("heater.start");
    lv_label_set_text(label, "Stop");
    // PS_PUB_NIL("ui.action.start");
    return;
  } else if (lv_strcmp(text, "Stop") == 0) {
    CreateStopConfirmation(label);
    // PS_PUB_INT("heater.state.set", heater::kStateOff);
    // lv_label_set_text(label, "Start");
    // Get the screen object if you passed it as user_data
    // ReflowScreen* screen = (ReflowScreen*)lv_event_get_user_data(e);
    // if (screen) {
    //   screen->StopConfirmation();
    // }
    // TODO: figure out stop confirmation
  } else if (lv_strcmp(text, "Mode") == 0) {
    CreateModeSwitcher(lv_display_get_screen_active(NULL));
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
  // lv_obj_t* backdrop = lv_obj_create(screen);

  ConfirmationContext ctx{.parent_screen = screen,
                          .backdrop = nullptr,
                          .object = label,
                          .title = "Stop?",
                          .message = "Are you sure you want to stop the heating process?",
                          .confirm_text = "Stop!",
                          .cancel_text = "Cancel",
                          .on_confirm =
                              [](void* obj) {
                                ConfirmationState* state = static_cast<ConfirmationState*>(obj);
                                FLOG_INFO("Stopping");
                                // PS_PUB_INT("heater.state.set", heater::kStateOff);
                                PS_PUB_NIL("heater.stop");
                                // lv_label_set_text(state->object, "Start");
                                // PS_PUB_NIL("ui.action.return");
                                if (state->backdrop) lv_obj_delete(state->backdrop);
                              },
                          .on_cancel =
                              [](void* obj) {
                                ConfirmationState* state = static_cast<ConfirmationState*>(obj);
                                // PS_PUB_NIL("ui.action.return");
                                FLOG_INFO("Cancelled Stopping");
                                if (state->backdrop) lv_obj_delete(state->backdrop);
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
    PS_PUB_INT("heater.state.set", heater::kStateOff);

    // PS_PUB_NIL("ui.action.stop");
    // return;
  } else if (lv_strcmp(text, "Cancel") == 0) {
  } else if (lv_strcmp(text, LV_SYMBOL_SETTINGS) == 0) {
    PS_PUB_NIL("ui.action.settings");
  }
  if (backdrop) {
    lv_obj_delete_async(backdrop);
  }
}

}  // namespace toothless