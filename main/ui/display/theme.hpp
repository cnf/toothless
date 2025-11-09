#pragma once

#include <esp_err.h>
#include <lvgl.h>

namespace toothless {
inline lv_theme_t* theme = nullptr;

esp_err_t SetTheme(lv_display_t* display);
}  // namespace toothless