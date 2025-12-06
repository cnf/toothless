#include "screenshot.hpp"

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include <cstdio>
#include <cstring>

#include "funlog.h"

namespace toothless {

void TakeSnapshot() {
  FLOG_INFO("Taking snapshot");
  lv_timer_create(
      +[](lv_timer_t* timer) {
        lv_draw_buf_t* snap = lv_snapshot_take(lv_screen_active(), LV_COLOR_FORMAT_ARGB8888);
        if (snap) {
          // Spawn task to write file
          xTaskCreate(
              [](void* arg) {
                auto* s = (lv_draw_buf_t*)arg;
                static int cnt = 0;
                char path[32];
                snprintf(path, sizeof(path), "/storage/scr_%03d.bmp", cnt++);
                SaveSnapshotBMP(s, path);
                lv_draw_buf_destroy(s);
                FLOG_INFO("Snapshot saved: %s", path);
                vTaskDelete(nullptr);
              },
              "snap_save", 4096, snap, 5, nullptr);
        }
        lv_timer_del(timer);
      },
      5 * 1000, nullptr);
}
/// Save lv_draw_buf_t as BMP to LittleFS
bool SaveSnapshotBMP(lv_draw_buf_t* snapshot, const char* path) {
  if (!snapshot || !path) return false;

  uint32_t w = snapshot->header.w;
  uint32_t h = snapshot->header.h;
  uint32_t row_size = ((w * 3 + 3) / 4) * 4;  // BMP rows are 4-byte aligned
  uint32_t img_size = row_size * h;
  uint32_t file_size = 54 + img_size;

  FILE* f = fopen(path, "wb");
  if (!f) return false;

  // BMP Header (14 bytes)
  uint8_t bmp_hdr[14] = {'B', 'M'};
  memcpy(&bmp_hdr[2], &file_size, 4);
  uint32_t offset = 54;
  memcpy(&bmp_hdr[10], &offset, 4);
  fwrite(bmp_hdr, 1, 14, f);

  // DIB Header (40 bytes)
  uint8_t dib_hdr[40] = {0};
  uint32_t dib_size = 40;
  memcpy(&dib_hdr[0], &dib_size, 4);
  memcpy(&dib_hdr[4], &w, 4);
  int32_t neg_h = -(int32_t)h;  // Top-down
  memcpy(&dib_hdr[8], &neg_h, 4);
  uint16_t planes = 1, bpp = 24;
  memcpy(&dib_hdr[12], &planes, 2);
  memcpy(&dib_hdr[14], &bpp, 2);
  memcpy(&dib_hdr[20], &img_size, 4);
  fwrite(dib_hdr, 1, 40, f);

  // Pixel data (ARGB8888 -> BGR)
  uint8_t* src = snapshot->data;
  uint8_t* row = (uint8_t*)malloc(row_size);
  for (uint32_t y = 0; y < h; y++) {
    memset(row, 0, row_size);
    for (uint32_t x = 0; x < w; x++) {
      uint32_t idx = (y * snapshot->header.stride) + x * 4;
      row[x * 3 + 0] = src[idx + 0];  // B
      row[x * 3 + 1] = src[idx + 1];  // G
      row[x * 3 + 2] = src[idx + 2];  // R
    }
    fwrite(row, 1, row_size, f);
  }
  free(row);
  fclose(f);
  return true;
}
}  // namespace toothless