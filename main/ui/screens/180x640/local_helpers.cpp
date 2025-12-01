#include "local_helpers.hpp"

#include "funlog.h"
#include "ui/themes/widget_factories.hpp"

namespace toothless {

void LocalTempRollerOpen(const NumberRollerContext& ctx) {
  // static size_t height = lv_display_get_vertical_resolution(ctx.parent_screen) * mult;
  // static size_t height = 60;

  FLOG_INFO("Open Target Roller");
  // if (_overlay_active) return;  // already active
  // _overlay_active = true;
  const char* digit_list =
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

  NumberRollerState* state = new NumberRollerState();
  state->target_spinbox = ctx.target_spinbox;
  state->on_confirm = ctx.on_confirm;

  state->backdrop = CreateBackdrop(ctx.parent_screen);

  lv_obj_t* col;

  col = ui::CreateRowContainer(state->backdrop);

  lv_obj_set_flex_align(col, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  lv_obj_set_size(col, lv_pct(100), LV_SIZE_CONTENT);
  // lv_obj_set_width(col, lv_pct(100));
  lv_obj_set_flex_grow(col, 1);

  state->col_a = ui::CreateSmallRoller(col, digit_list, 0);
  lv_obj_add_event_cb(state->col_a, TimeRollerHandler, LV_EVENT_ALL, state);

  state->col_b = ui::CreateSmallRoller(col, digit_list, 0);
  lv_obj_add_event_cb(state->col_b, TimeRollerHandler, LV_EVENT_ALL, state);

  state->col_c = ui::CreateSmallRoller(col, digit_list, 0);
  lv_obj_add_event_cb(state->col_c, TimeRollerHandler, LV_EVENT_ALL, state);

  {
    col = ui::CreateRowContainer(state->backdrop);

    lv_obj_set_width(col, lv_pct(100));
    lv_obj_set_height(col, LV_SIZE_CONTENT);
    lv_obj_set_flex_grow(col, 0);

    lv_obj_t* ok_btn = ui::CreatePrimaryButton(col, "Set Target", lv_pct(100), NULL, true);
    lv_obj_add_event_cb(ok_btn, LocalTempRollerCleanupHandler, LV_EVENT_CLICKED, state);
  }
}

void LocalTempRollerCleanupHandler(lv_event_t* e) {
  FLOG_INFO("Temp Roller cleanup started");
  NumberRollerState* state = (NumberRollerState*)lv_event_get_user_data(e);

  if (!state) {
    FLOG_ERROR("no state passed... propably have a memory leak...");
    return;
  }
  // On apply, read value and close keyboard
  char buf[32];
  lv_roller_get_selected_str(state->col_a, buf, sizeof(buf));
  int hours = atoi(buf);
  lv_roller_get_selected_str(state->col_b, buf, sizeof(buf));
  int minutes = atoi(buf);
  lv_roller_get_selected_str(state->col_c, buf, sizeof(buf));
  int seconds = atoi(buf);
  FLOG_INFO("Got numbers: %d%d%d", hours, minutes, seconds);
  int32_t total_seconds = (hours * 100 + minutes * 10 + seconds);  // TODO: units

  if (state->on_confirm) {
    FLOG_INFO("Running Callback");
    state->on_confirm(total_seconds);
    FLOG_INFO("Done");
  }

  if (state->backdrop) lv_obj_delete(state->backdrop);
  state->backdrop = nullptr;
  if (state->col_a) lv_obj_delete(state->col_a);
  state->col_a = nullptr;
  if (state->col_b) lv_obj_delete(state->col_b);
  state->col_b = nullptr;
  if (state->col_c) lv_obj_delete(state->col_c);
  state->col_c = nullptr;
  FLOG_INFO("All done");

  delete state;
}

}  // namespace toothless