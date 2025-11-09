#pragma once

#include "config.h"

#include "MCP23008.hpp"
#include <cstdint>
#include <memory>

class IOManager {
public:
  IOManager();
  ~IOManager() = default;
  void Setup();
  int GetLevel(uint8_t number);
  esp_err_t SetLevel(uint8_t number, bool level);
  esp_err_t SetDirection(uint8_t number, gpio_mode_t mode);

  static std::shared_ptr<IOManager> GetInstance() {
    static std::shared_ptr<IOManager> instance(new IOManager());
    return instance;
  }

private:
  std::unique_ptr<MCP23008> _port_expander;
  IOManager(const IOManager &) = delete;
  IOManager &operator=(const IOManager &) = delete;
};