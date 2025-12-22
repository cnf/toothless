#pragma once
#include <driver/gpio.h>

#include <cstddef>
#include <cstdint>

#include "sdkconfig.h"

namespace impl {
static constexpr gpio_num_t kGpioFreeList[] = {
    GPIO_NUM_17,  //<!
    GPIO_NUM_18,  //<!
    GPIO_NUM_43,  //<!
    GPIO_NUM_44,  //<!
};

static constexpr int32_t kHRes = 180;
static constexpr int32_t kVRes = 640;
static constexpr size_t kLcdDPI = 195;  //<! display dpi

}  // namespace impl