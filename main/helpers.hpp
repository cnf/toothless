#pragma once

#include "config.h"

#include "funlog.h"
#include <esp_heap_caps.h>
#include <esp_system.h>

inline void MemPressure() {
  FLOG_INFO("Free Heap: %li", esp_get_free_heap_size());
  FLOG_INFO("Minimum Free Heap: %li", esp_get_minimum_free_heap_size());
  FLOG_INFO("Largest Free Block: %li", heap_caps_get_largest_free_block(MALLOC_CAP_DMA));
}