#pragma once

#include <esp_err.h>
#include <lvgl.h>

namespace display {
namespace impl {

esp_err_t SetupIndev(lv_display_t* disp);
void LvglTouchCallback(lv_indev_t* indev, lv_indev_data_t* data);

}  // namespace impl
}  // namespace display