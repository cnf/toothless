#include "ui/screens/screen_sizes.hpp"

#include <esp_err.h>

#include "funlog.h"
#include "ui/display/display.hpp"

namespace toothless {
namespace sizes {
esp_err_t CreateSizes() {
  FLOG_INFO("Setting up screen sizes...");
  return ESP_OK;
}
}  // namespace sizes
}  // namespace toothless