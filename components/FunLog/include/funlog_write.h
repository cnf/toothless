#pragma once
#ifdef __cplusplus
extern "C" {
#endif
#include <esp_log.h>

typedef int (*net_fun_log_t)(int level, const char *tag, const char *fmt, va_list args);

net_fun_log_t funlog_set_net_fun_log(net_fun_log_t func);

int net_fun_log(int level, const char *tag, const char *fmt, va_list args);

void net_fun_log_writev(int level, const char *tag, const char *fmt, va_list args);

void net_fun_log_write(int level, const char *tag, const char *fmt, ...);

#ifdef __cplusplus
}
#endif