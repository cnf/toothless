#pragma once

#include <esp_chip_info.h>
#include <esp_heap_caps.h>
#include <esp_system.h>
#include <soc/rtc.h>

#include "config.h"
#include "funlog.h"

static esp_chip_info_t chip_info;

struct ChipInfo {
  std::string model;
  bool flash;
  bool wifi;
  bool ble;
  bool bt;
  bool IEEE802154;
  bool embedded_psram;
  uint16_t revision;
  uint8_t cores;
};

inline void MemPressure() {
  FLOG_INFO("Free Heap: %li", esp_get_free_heap_size());
  FLOG_INFO("Minimum Free Heap: %li", esp_get_minimum_free_heap_size());
  FLOG_INFO("Largest Free Block: %li", heap_caps_get_largest_free_block(MALLOC_CAP_DMA));
}

inline const char* GetChipModel() {
  if (!chip_info.model) esp_chip_info(&chip_info);
  switch (chip_info.model) {
    case CHIP_ESP32:
      return "ESP32";
    case CHIP_ESP32S2:
      return "ESP32-S2";
    case CHIP_ESP32S3:
      return "ESP32-S3";
    case CHIP_ESP32C3:
      return "ESP32-C3";
    case CHIP_ESP32C2:
      return "ESP32-C2";
    case CHIP_ESP32C6:
      return "ESP32-C6";
    case CHIP_ESP32H2:
      return "ESP32-H2";
    case CHIP_ESP32P4:
      return "ESP32-P4";
    case CHIP_POSIX_LINUX:
      return "POSIX/Linux Simulator";
    default:
      return "Unknown Model";
  }
}
inline esp_err_t GetChipInfo(ChipInfo& info) {
  if (!chip_info.model) esp_chip_info(&chip_info);
  rtc_cpu_freq_config_t freq_config;
  rtc_clk_cpu_freq_get_config(&freq_config);
  info.model = GetChipModel();
  info.flash = chip_info.features & CHIP_FEATURE_EMB_FLASH;
  info.wifi = chip_info.features & CHIP_FEATURE_WIFI_BGN;
  info.ble = chip_info.features & CHIP_FEATURE_BLE;
  info.bt = chip_info.features & CHIP_FEATURE_BT;
  info.IEEE802154 = chip_info.features & CHIP_FEATURE_IEEE802154;
  info.embedded_psram = chip_info.features & CHIP_FEATURE_EMB_PSRAM;
  info.revision = chip_info.revision;
  info.cores = chip_info.cores;
  return ESP_OK;
}
