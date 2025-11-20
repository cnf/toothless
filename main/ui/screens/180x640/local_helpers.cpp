#include "local_helpers.hpp"

#include "funlog.h"

namespace toothless {

lv_obj_t* LocalCreateBottomRow(lv_obj_t* container) {
  static size_t height = 60;  //<! 1/30 of 180px
  static lv_obj_t* start_stop;
  lv_obj_t* wrapper = lv_obj_create(container);
  lv_obj_remove_style_all(wrapper);
  lv_obj_set_style_bg_opa(wrapper, LV_OPA_TRANSP, 0);
  lv_obj_set_style_pad_all(wrapper, 0, 0);
  lv_obj_set_scrollbar_mode(wrapper, LV_SCROLLBAR_MODE_OFF);
  lv_obj_remove_flag(wrapper, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_size(wrapper, lv_pct(100), height);
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

void LocalTempRollerOpen(const TimeRollerContext& ctx) {
  // static size_t height = lv_display_get_vertical_resolution(ctx.parent_screen) * mult;
  static size_t height = 60;

  FLOG_INFO("Open Time Roller");
  // if (_overlay_active) return;  // already active
  // _overlay_active = true;
  const char* minsecstr =
      "0\n"
      "1\n"
      "2\n"
      "3\n"
      "4\n"
      "5\n"
      "6\n"
      "7\n"
      "8\n"
      "9";

  // create a small textarea + keyboard

  TimeRollerState* state = new TimeRollerState();
  // state->backdrop = ctx.backdrop;
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
    // 100
    state->roller_hours = lv_roller_create(col);
    lv_roller_set_options(state->roller_hours, minsecstr, LV_ROLLER_MODE_INFINITE);

    lv_roller_set_visible_row_count(state->roller_hours, 4);
    lv_obj_center(state->roller_hours);
    lv_obj_add_event_cb(state->roller_hours, TimeRollerHandler, LV_EVENT_ALL, state);
  }
  {
    // 10
    state->roller_minutes = lv_roller_create(col);
    lv_roller_set_options(state->roller_minutes, minsecstr, LV_ROLLER_MODE_INFINITE);

    lv_roller_set_visible_row_count(state->roller_minutes, 4);
    lv_obj_center(state->roller_minutes);
    lv_obj_add_event_cb(state->roller_minutes, TimeRollerHandler, LV_EVENT_ALL, state);
  }
  {
    // 1
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
    lv_obj_set_size(ok_btn, lv_pct(100), height);
    lv_obj_t* ok_label = lv_label_create(ok_btn);
    lv_label_set_text(ok_label, "Set Target");
    lv_obj_center(ok_label);
    lv_obj_add_event_cb(ok_btn, LocalTempRollerCleanupHandler, LV_EVENT_CLICKED, state);
  }
}

void LocalTempRollerCleanupHandler(lv_event_t* e) {
  FLOG_INFO("Temp Roller cleanup started");
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
  FLOG_INFO("Got numbers: %d%d%d", hours, minutes, seconds);
  int32_t total_seconds = (hours * 100 + minutes * 10 + seconds) * 100;  // TODO: units

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

}  // namespace toothless