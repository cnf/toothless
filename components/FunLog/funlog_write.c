#ifdef __cplusplus
extern "C" {
#endif
#include "funlog_write.h"
#include <esp_log.h>
#include <esp_log_level.h>
#include <stdio.h>

static net_fun_log_t _net_fun_log_func = &net_fun_log;

int net_fun_log(int level, const char *tag, const char *fmt, va_list args) { return 0; }

net_fun_log_t funlog_set_net_fun_log(net_fun_log_t func) {
  // esp_log_impl_lock(); //FIXME: implement lock
  net_fun_log_t orig_func = _net_fun_log_func;
  _net_fun_log_func = func;
  // esp_log_impl_unlock();
  return orig_func;
}

void net_fun_log_writev(int level, const char *tag, const char *fmt, va_list args) {
  // FIXME: figure out logging levels for net log
  // esp_log_level_t level_for_tag = esp_log_level_get_timeout(tag);
  // if (ESP_LOG_NONE != level_for_tag && level <= level_for_tag) {
  if (!_net_fun_log_func || !tag || !fmt) {
    printf("funlog: null ptr (func: %p tag: %p fmt: %p)\n", _net_fun_log_func, tag, fmt);
    return;
  }
  // printf("funlog ptr: %p tag: %s fmt: %s\n", _net_fun_log_func, tag, fmt);
  (*_net_fun_log_func)(level, tag, fmt, args);
}

void net_fun_log_write(int level, const char *tag, const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  net_fun_log_writev(level, tag, fmt, args);
  va_end(args);
}

#ifdef __cplusplus
}
#endif