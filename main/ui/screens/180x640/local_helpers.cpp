#include "local_helpers.hpp"

#include "funlog.h"
#include "ui/screens/overlay_manager.hpp"
#include "ui/themes/widget_factories.hpp"

namespace toothless {

void LocalTempRollerOpen(const NumberRollerContext& ctx) {
  FLOG_INFO("Open Target Roller");
  auto& mgr = OverlayManager::Instance();
  if (mgr.IsActive()) return;

  auto* state = mgr.Open<NumberRollerState>(ctx.parent_screen);
  state->on_confirm = ctx.on_confirm;
  state->target_spinbox = ctx.target_spinbox;

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

  if (state->on_confirm) state->on_confirm(total_seconds);
  OverlayManager::Instance().Close();  // Single cleanup path
  FLOG_INFO("All done");

  delete state;
}

}  // namespace toothless