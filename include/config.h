#pragma once

#include "sdkconfig.h"
#include <cstdint>

#define BSP_CAPS_DISPLAY 1
#define BSP_CAPS_TOUCH 1

// #define BSP_FEATURE_LCD

// #define BSP_LCD_HOST SPI2_HOST
#define BSP_LCD_H_RES 320
#define BSP_LCD_V_RES 480
#define BSP_SD_SPI_HOST VSPI_HOST
#define BSP_SD_SPI_MOSI GPIO_NUM_23
#define BSP_SD_SPI_MISO GPIO_NUM_19
#define BSP_SD_SPI_SCLK GPIO_NUM_18
#define BSP_SD_SPI_CS GPIO_NUM_5

#define DISPAY_DC_GPIO GPIO_NUM_21
#define DISPAY_RESET_GPIO GPIO_NUM_22
#define DISPAY_BL_GPIO GPIO_NUM_4

// #define TOUCH_CS_GPIO GPIO_NUM_15
// #define TOUCH_INT_GPIO GPIO_NUM_34
// #define TOUCH_RST_GPIO GPIO_NUM_33

#define LVGL_TICK_PERIOD_MS 2
// #define LV_USE_DEMO_MUSIC 1

// #define MAX6675_CLK_GPIO GPIO_NUM_14
// #define MAX6675_CS_GPIO GPIO_NUM_27
// #define MAX6675_MISO_GPIO GPIO_NUM_12

static constexpr uint16_t kUIMaxTargetTemperatureC = 999;

#define kHeaterControlPin 12;