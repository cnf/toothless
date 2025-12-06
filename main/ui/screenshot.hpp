#pragma once

#include <lvgl.h>

#include <cstdint>

namespace toothless {
void TakeSnapshot();

bool SaveSnapshotBMP(lv_draw_buf_t* snapshot, const char* path);
}  // namespace toothless