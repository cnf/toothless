#include "io_manager.hpp"
#include "funlog.h"

IOManager::IOManager() { _port_expander = std::make_unique<MCP23008>(); }

void IOManager::Setup() { _port_expander->Init(); }

static inline uint8_t StripExternalFlag(uint8_t n) { return n & 0x7F; }

int IOManager::GetLevel(uint8_t number) {
  uint8_t pin = StripExternalFlag(number);
  if (pin >= 8) {
    FLOG_ERROR("GetLevel bad pin %u", pin);
    return -1;
  }
  return _port_expander->GpioGetLevel(pin);
  return ESP_OK;
}

esp_err_t IOManager::SetLevel(uint8_t number, bool level) {
  uint8_t pin = StripExternalFlag(number);
  if (pin >= 8) {
    FLOG_ERROR("SetLevel bad pin %u", pin);
    return ESP_ERR_INVALID_ARG;
  }
  return _port_expander->GpioSetLevel(pin, level);
}

esp_err_t IOManager::SetDirection(uint8_t number, gpio_mode_t mode) {
  uint8_t pin = StripExternalFlag(number);
  if (pin >= 8) {
    FLOG_ERROR("SetDirection bad pin %u", pin);
    return ESP_ERR_INVALID_ARG;
  }
  return _port_expander->GpioSetDirection(pin, mode);
}
