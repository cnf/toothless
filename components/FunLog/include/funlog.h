#pragma once
#undef FLOG_ERROR
#undef FLOG_WARN
#undef FLOG_INFO
#undef FLOG_DEBUG
#undef FLOG_TRACE
// #undef ASSERT
// #undef ASSERTM

#include "funlog_write.h"
#include <esp_log.h>
#include <string.h>

#ifndef ESP_LOG_LEVEL
#define ESP_LOG_LEVEL ESP_LOG_INFO
#endif

// We are porting https://github.com/hideakitai/DebugLog/blob/main/DebugLogEnable.h
// also see https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-guides/error-handling.html

#define FLOG_SHORT_FILENAME                                                                                            \
  (strrchr(__FILE__, '/')    ? strrchr(__FILE__, '/') + 1                                                              \
   : strrchr(__FILE__, '\\') ? strrchr(__FILE__, '\\') + 1                                                             \
                             : __FILE__)

#define FLOG_PREAMBLE "L.%d [%s]: "

#define FLOGGER(level, tag, format, ...)                                                                               \
  do {                                                                                                                 \
    ESP_LOG_LEVEL_LOCAL(level, tag, FLOG_PREAMBLE format, __LINE__, __func__, ##__VA_ARGS__);                          \
    net_fun_log_write(level, tag, FLOG_PREAMBLE format, __LINE__, __func__, ##__VA_ARGS__);                            \
  } while (0)

#define FLOG_ERROR(format, ...) FLOGGER(ESP_LOG_ERROR, FLOG_SHORT_FILENAME, format, ##__VA_ARGS__)
#define FLOG_WARN(format, ...) FLOGGER(ESP_LOG_WARN, FLOG_SHORT_FILENAME, format, ##__VA_ARGS__)
#define FLOG_INFO(format, ...) FLOGGER(ESP_LOG_INFO, FLOG_SHORT_FILENAME, format, ##__VA_ARGS__)
#define FLOG_DEBUG(format, ...) FLOGGER(ESP_LOG_DEBUG, FLOG_SHORT_FILENAME, format, ##__VA_ARGS__)

#define FLOG_TRACE(format, ...) ESP_LOGV(FLOG_SHORT_FILENAME, FLOG_PREAMBLE format, __LINE__, __func__, ##__VA_ARGS__)
#define FLOG_VERBOSE(format, ...) ESP_LOGV(FLOG_SHORT_FILENAME, FLOG_PREAMBLE format, __LINE__, __func__, ##__VA_ARGS__)

#define FLOGN_ERROR(format, ...) ESP_LOGE(FLOG_SHORT_FILENAME, FLOG_PREAMBLE format, __LINE__, __func__, ##__VA_ARGS__)
#define FLOGN_WARN(format, ...) ESP_LOGW(FLOG_SHORT_FILENAME, FLOG_PREAMBLE format, __LINE__, __func__, ##__VA_ARGS__)
#define FLOGN_INFO(format, ...) ESP_LOGI(FLOG_SHORT_FILENAME, FLOG_PREAMBLE format, __LINE__, __func__, ##__VA_ARGS__)
#define FLOGN_DEBUG(format, ...) ESP_LOGD(FLOG_SHORT_FILENAME, FLOG_PREAMBLE format, __LINE__, __func__, ##__VA_ARGS__)
#define FLOGN_VERBOSE(format, ...)                                                                                     \
  ESP_LOGV(FLOG_SHORT_FILENAME, FLOG_PREAMBLE format, __LINE__, __func__, ##__VA_ARGS__)
// #include <cassert>
// #define ASSERT(b) assert(b)
// #define ASSERTM(b, msg) assert((msg, b))
