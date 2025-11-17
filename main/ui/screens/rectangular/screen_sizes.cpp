#include "ui/screens/screen_sizes.hpp"

#include <esp_err.h>

#include "funlog.h"
#include "ui/display/display.hpp"

namespace toothless {
namespace sizes {
esp_err_t CreateSizes() {
  FLOG_INFO("Setting up screen sizes...");
  // if (!Display::IsTall()) {
  //   FLOG_INFO("Using sizes for smoll display");
  //   // LV_FONT_DECLARE(AdwaitaMonoB_28);
  //   // LV_FONT_DECLARE(AdwaitaMonoB_32);
  //   // LV_FONT_DECLARE(AdwaitaMonoB_48);
  //   font::numbers_small = lv_font_montserrat_22;
  //   font::numbers_medium = lv_font_montserrat_24;
  //   font::numbers_large = AdwaitaMonoB_28;
  // }
  return ESP_OK;
}
}  // namespace sizes
}  // namespace toothless