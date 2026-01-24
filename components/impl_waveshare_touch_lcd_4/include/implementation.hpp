#pragma once
#include <driver/gpio.h>

#include <cstddef>
#include <cstdint>

#include "sdkconfig.h"

namespace impl {

#if defined(CONFIG_IO_EXPANDER_ENABLE_GPIO_API_WRAPPER)
static constexpr gpio_num_t XP_NUM_00 = static_cast<gpio_num_t>(GPIO_NUM_MAX + 0);
static constexpr gpio_num_t XP_NUM_01 = static_cast<gpio_num_t>(GPIO_NUM_MAX + 1);
static constexpr gpio_num_t XP_NUM_02 = static_cast<gpio_num_t>(GPIO_NUM_MAX + 2);
static constexpr gpio_num_t XP_NUM_03 = static_cast<gpio_num_t>(GPIO_NUM_MAX + 3);
static constexpr gpio_num_t XP_NUM_04 = static_cast<gpio_num_t>(GPIO_NUM_MAX + 4);
static constexpr gpio_num_t XP_NUM_05 = static_cast<gpio_num_t>(GPIO_NUM_MAX + 5);
static constexpr gpio_num_t XP_NUM_06 = static_cast<gpio_num_t>(GPIO_NUM_MAX + 6);
static constexpr gpio_num_t XP_NUM_07 = static_cast<gpio_num_t>(GPIO_NUM_MAX + 7);
static constexpr gpio_num_t kLcdTouchResetPin = XP_NUM_01;
static constexpr gpio_num_t kLcdBacklightPin = XP_NUM_02;
static constexpr gpio_num_t kLcdResetPin = XP_NUM_03;
static constexpr gpio_num_t kSystemEnablePin = XP_NUM_05;
static constexpr gpio_num_t kBeeperEnablePin = XP_NUM_06;
static constexpr gpio_num_t kRtcInterruptPin = XP_NUM_07;
#endif

#define EXP_LCD_TOUCH_RST (IO_EXPANDER_PIN_NUM_1)
#define EXP_LCD_BACKLIGHT_PIN (IO_EXPANDER_PIN_NUM_2)
#define EXP_LCD_RST (IO_EXPANDER_PIN_NUM_3)
#define EXP_SYS_EN (IO_EXPANDER_PIN_NUM_5)
#define EXP_BEE_EN (IO_EXPANDER_PIN_NUM_6)
#define EXP_RTC_INT (IO_EXPANDER_PIN_NUM_7)

static constexpr gpio_num_t kGpioFreeList[] = {
    GPIO_NUM_NC,  //<!
};

static constexpr int32_t kHRes = 480;
static constexpr int32_t kVRes = 480;
static constexpr size_t kLcdDPI = 169;  //<! display dpi

}  // namespace impl