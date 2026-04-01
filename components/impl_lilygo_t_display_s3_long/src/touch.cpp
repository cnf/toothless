// cSpell: words lvgl indev
//
// AXS15231B touch for LilyGo T-Display S3 Long.
// I2C @ 0x3B, SDA/SCL from Kconfig (15/10).
// Native portrait coords rotated to landscape for LVGL.
//
#include <esp_check.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "display_impl.hpp"
#include "funlog.h"
#include "i2c_manager.hpp"

static const char* TAG = FLOG_SHORT_FILENAME;

namespace impl {
namespace display {

// ---------------------------------------------------------------------------
// Internal state
// ---------------------------------------------------------------------------
static std::shared_ptr<I2cManager> _i2c;
static i2c_master_dev_handle_t _touch_dev = nullptr;

/// AXS15231B touch read command (8 bytes)
static const uint8_t kTouchReadCmd[] = {0xb5, 0xab, 0xa5, 0x5a, 0x00, 0x00, 0x00, 0x08};

/// Response length: 2-point data = 14 bytes
static constexpr size_t kTouchResponseLen = 14;

// ---------------------------------------------------------------------------
// Touch data helpers
// ---------------------------------------------------------------------------

/// Read raw touch data via atomic I2C transmit-receive.
/// Returns number of active touch points (0 = none).
static uint8_t TouchGetData(int16_t* x, int16_t* y) {
  uint8_t buf[kTouchResponseLen] = {};

  if (_i2c->TransmitReceive(_touch_dev, kTouchReadCmd, sizeof(kTouchReadCmd), buf, kTouchResponseLen) != ESP_OK) {
    return 0;
  }

  uint8_t num = AXS_GET_POINT_NUM(buf);
  uint16_t gesture = AXS_GET_GESTURE_TYPE(buf);

  /// Valid touch: has points and no gesture
  if (num == 0 || gesture != 0) {
    return 0;
  }

  *x = static_cast<int16_t>(AXS_GET_POINT_X(buf, 0));
  *y = static_cast<int16_t>(AXS_GET_POINT_Y(buf, 0));
  return num;
}

// ---------------------------------------------------------------------------
// LVGL indev callback
// ---------------------------------------------------------------------------

void LvglTouchCallback(lv_indev_t* /*indev*/, lv_indev_data_t* data) {
  int16_t raw_x = 0;
  int16_t raw_y = 0;

  if (TouchGetData(&raw_x, &raw_y) > 0) {
    /// Panel reports x on long axis (0-639), y on short axis (0-179).
    /// LVGL landscape display is 640 wide x 180 tall.
    /// Match flush rotation: lvgl_x = (639 - raw_x), lvgl_y = raw_y
    data->point.x = raw_x;
    data->point.y = raw_y;
    data->state = LV_INDEV_STATE_PRESSED;
  } else {
    data->state = LV_INDEV_STATE_RELEASED;
  }
}

// ---------------------------------------------------------------------------
// Setup
// ---------------------------------------------------------------------------

esp_err_t TouchPanelSetup() {
  LV_LOG_USER("Init AXS15231B touch (I2C @ 0x%02X)", kTouchI2cAddress);

  _i2c = I2cManager::GetInstance();
  if (!_i2c->IsInitialized()) {
    ESP_RETURN_ON_ERROR(_i2c->Init(), TAG, "I2C bus init failed");
  }

  ESP_RETURN_ON_ERROR(_i2c->Probe(kTouchI2cAddress), TAG, "Touch not found at 0x%02X", kTouchI2cAddress);
  LV_LOG_USER("Touch controller found");

  i2c_device_config_t dev_cfg = {
      .dev_addr_length = I2C_ADDR_BIT_LEN_7,
      .device_address = kTouchI2cAddress,
      .scl_speed_hz = I2cManager::kClockSpeedHz,
  };
  ESP_RETURN_ON_ERROR(_i2c->AddDevice(&dev_cfg, &_touch_dev), TAG, "Failed to add touch device");

  lv_indev_t* indev = lv_indev_create();
  if (!indev) {
    LV_LOG_ERROR("Failed to create LVGL indev");
    return ESP_ERR_NO_MEM;
  }
  lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
  lv_indev_set_display(indev, GetDisplayObjPtr());
  lv_indev_set_read_cb(indev, LvglTouchCallback);

  LV_LOG_USER("Touch ready (landscape: 640x180)");
  return ESP_OK;
}

/// Unused - LVGL polls via indev callback directly
static void TouchPollTask(void*) {}

}  // namespace display
}  // namespace impl
