#pragma once
#include <driver/gpio.h>

#include <cstddef>
#include <cstdint>

#include "sdkconfig.h"

namespace impl {

static constexpr gpio_num_t kGpioFreeList[] = {
    GPIO_NUM_17,  //<!

    // GPIO_NUM_3,   //<!
    // GPIO_NUM_4,   //<!
    // GPIO_NUM_5,   //<!
    // GPIO_NUM_6,   //<!
    // GPIO_NUM_7,   //<!
    // GPIO_NUM_8,   //<!
    // GPIO_NUM_38,  //<!
    // GPIO_NUM_39,  //<!
    // GPIO_NUM_40,  //<!
    // GPIO_NUM_41,  //<!
    // GPIO_NUM_42,  //<!
    // GPIO_NUM_43,  //<! U0TXD Qwiic
    // GPIO_NUM_44,  //<! U0RXD Qwiic
    // GPIO_NUM_45,  //<!
    // GPIO_NUM_46,  //<!
    // GPIO_NUM_47,  //<!
    // GPIO_NUM_48,  //<!
};

static constexpr int32_t kHRes = 800;
static constexpr int32_t kVRes = 480;
static constexpr size_t kLcdDPI = 217;  //<! display dpi

}  // namespace impl