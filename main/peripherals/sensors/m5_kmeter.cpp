#include "m5_kmeter.hpp"

#include <esp_check.h>
#include <esp_timer.h>

#include "funlog.h"
#include "helpers/rolling_average.hpp"
#include "helpers/string_to_snake.hpp"
#include "peripherals/peripheral_registry.hpp"

extern "C" {
#include <pubsub.h>
}

namespace toothless {

const PeripheralInfo M5KMeter::_info = {kM5KMeterName, "temperature", kM5KMeterBusType, kMeterDefaultAddr};

static bool s_registered = []() {
  PeripheralRegistry::Register(
      {.info = M5KMeter::GetInfo(), .probe = M5KMeter::Detect, .create = []() { return M5KMeter::GetInstance(); }});
  return true;
}();

M5KMeter::M5KMeter() {
  _i2c_mgr = I2cManager::GetExternalInstance();
  i2c_device_config_t device_config = {
      .dev_addr_length = I2C_ADDR_BIT_LEN_7,
      .device_address = kMeterDefaultAddr,
      .scl_speed_hz = I2cManager::kClockSpeedHz,
  };
  ESP_ERROR_CHECK_WITHOUT_ABORT(_i2c_mgr->AddDevice(&device_config, &_dev_handle));
  _initialized = true;
}

M5KMeter::~M5KMeter() {
  if (_dev_handle != nullptr) {
    // _i2c_mgr->RemoveDevice(_dev_handle);
    _dev_handle = nullptr;
  }
}

bool M5KMeter::Detect() {
  if (I2cManager::GetExternalInstance()->Probe(kMeterDefaultAddr) == ESP_OK) {
    FLOG_INFO("M5 KMeter detected at address 0x%02X", kMeterDefaultAddr);
    return true;
  }
  FLOG_ERROR("M5 KMeter not detected at address 0x%02X", kMeterDefaultAddr);
  return false;
};

esp_err_t M5KMeter::Init() {
  _topic = std::format("{}.{}.{}.{}", topics::peripherals::sensors::temperature, BusTypeToString(kM5KMeterBusType),
                       std::to_string(kMeterDefaultAddr), StringToSnake(kM5KMeterName));
  FLOG_INFO("M5 KMeter topic: %s", _topic.c_str());
  // Initialization code if needed
  FLOG_INFO("M5 KMeter initialized");
  return ESP_OK;
}

esp_err_t M5KMeter::Loop() {
  static uint32_t last = 0;
  if (!_initialized) {
    FLOG_ERROR("M5 KMeter not initialized");
    return ESP_ERR_INVALID_STATE;
  };
  if (esp_timer_get_time() / 1000 - last < kTemperatureReadIntervalMs) {
    return ESP_OK;
  }

  uint32_t temp;
  esp_err_t err = ReadCelsius(temp);
  if (err != ESP_OK) {
    FLOG_ERROR("Failed to get temperature: %s", esp_err_to_name(err));
    return err;
  };
  _avg.Add(temp);
  PS_PUB_INT(_topic.c_str(), _avg.Get());
  PS_PUB_INT("sensor.temperature.zone", _avg.Get());
  last = esp_timer_get_time() / 1000;
  return ESP_OK;
}

esp_err_t M5KMeter::ReadCelsius(uint32_t& celsius) {
  int32_t res = 0;

  ESP_RETURN_ON_ERROR(_i2c_mgr->ReadRegister(_dev_handle, kMeterRegTempertureValue, (uint8_t*)&res, 4),
                      FLOG_SHORT_FILENAME, "Failed to read temperature value from M5 KMeter");
  // readBytes(_addr, kMeterRegTempertureValue, (uint8_t*)&res, 4);
  // FLOG_INFO("M5 KMeter raw temperature value: %li", res);
  celsius = res;
  return ESP_OK;
}
}  // namespace toothless