#pragma once

#include <lvgl.h>

#include <functional>
#include <optional>
#include <string>

#include "impl_config.hpp"
#include "ui/screens/screen_helpers.hpp"
// #include "ui/display/display.hpp"

namespace toothless {

lv_obj_t* LocalCreateBottomRow(lv_obj_t* container);

void LocalTempRollerOpen(const TimeRollerContext& ctx);

void LocalTempRollerCleanupHandler(lv_event_t* e);

}  // namespace toothless