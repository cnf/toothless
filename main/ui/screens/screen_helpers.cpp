#include "ui/screens/screen_helpers.hpp"

#include <lvgl.h>

#include "config.h"
#include "screen_helpers.hpp"

namespace toothless {

lv_obj_t* CreateBackdrop(lv_obj_t* screen) {
  lv_obj_t* backdrop = lv_obj_create(screen);
  lv_obj_set_size(backdrop, lv_pct(100), lv_pct(100));
  lv_obj_set_style_pad_all(backdrop, 0, 0);  // Global 2% border
  lv_obj_set_pos(backdrop, 0, 0);
  lv_obj_remove_flag(backdrop, LV_OBJ_FLAG_FLEX_IN_NEW_TRACK);
  lv_obj_add_flag(backdrop, LV_OBJ_FLAG_FLOATING);
  // Set screen to vertical flex layout
  lv_obj_set_layout(backdrop, LV_LAYOUT_FLEX);
  lv_obj_set_flex_flow(backdrop, LV_FLEX_FLOW_COLUMN);  // Vertical stacking
  lv_obj_set_style_pad_gap(backdrop, 10, 0);            // 10px gap between items
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

void ConfirmationPopup(const ConfirmationContext& ctx) {
  FLOG_INFO("Open Confirmation");

  if (ctx.backdrop) return;  // already a backdrop active
  FLOG_INFO("Confirming...");

  ConfirmationState* state = new ConfirmationState();
  state->backdrop = ctx.backdrop;

  state->backdrop = CreateBackdrop(ctx.parent_screen);
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
    if (state->on_confirm) state->on_confirm();
  } else if (strcmp(text, state->cancel_text.c_str()) == 0) {
    FLOG_DEBUG("Canceled");
    if (state->on_cancel) state->on_cancel();
  }
  if (state->backdrop) {
    lv_obj_delete_async(state->backdrop);
  }
}

}  // namespace toothless