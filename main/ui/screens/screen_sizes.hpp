#pragma once

#include <esp_err.h>
#include <lvgl.h>

LV_FONT_DECLARE(AdwaitaMonoB_96);
LV_FONT_DECLARE(AdwaitaMonoB_64);
LV_FONT_DECLARE(AdwaitaMonoB_48);
LV_FONT_DECLARE(AdwaitaMonoB_32);
LV_FONT_DECLARE(AdwaitaMonoB_28);

namespace toothless {
namespace sizes {
namespace font {

#if CONFIG_IMPL_WSRPIG_VRES >= 400
static lv_font_t tiny = lv_font_montserrat_12;
static lv_font_t small = lv_font_montserrat_18;
static lv_font_t medium = lv_font_montserrat_22;
static lv_font_t large = lv_font_montserrat_32;
static lv_font_t xlarge = lv_font_montserrat_48;
static lv_font_t numbers_small = AdwaitaMonoB_32;
static lv_font_t numbers_medium = AdwaitaMonoB_48;
static lv_font_t numbers_large = AdwaitaMonoB_96;
// #elsif CONFIG_IMPL_WSRPIG_VRES >= 300
#else
static lv_font_t tiny = lv_font_montserrat_10;
static lv_font_t small = lv_font_montserrat_16;
static lv_font_t medium = lv_font_montserrat_20;
static lv_font_t large = lv_font_montserrat_24;
static lv_font_t xlarge = lv_font_montserrat_32;
static lv_font_t numbers_small = AdwaitaMonoB_28;
static lv_font_t numbers_medium = AdwaitaMonoB_32;
static lv_font_t numbers_large = AdwaitaMonoB_48;
#endif
}  // namespace font

static constexpr float kBottomRowHeightPercentTall =
    0.2f;  ///< Bottom row height as percentage of screen height for tall displays
static constexpr float kBottomRowHeightPercentShort =
    0.15f;  ///< Bottom row height as percentage of screen height for wide displays
static constexpr float kButtonSizePercentTall = 0.2f;  ///< Button size as percentage of screen height for tall displays
static constexpr float kButtonSizePercentSlim =
    0.15f;  ///< Button size as percentage of screen height for wide displays
static constexpr float kTemperatureSectionHeightPercentTall =
    0.2f;  ///< Temperature section height as percentage of screen height for tall displays
static constexpr float kTemperatureSectionHeightPercentSlim =
    0.15f;  ///< Temperature section height as percentage of screen height for wide displays
esp_err_t CreateSizes();
}  // namespace sizes
}  // namespace toothless