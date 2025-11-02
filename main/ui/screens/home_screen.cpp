#include "ui/screens/home_screen.hpp"

#include "funlog.h"

namespace toothless {

HomeScreen::HomeScreen() { _objects = std::make_unique<HomeScreenLabels>(); }

lv_obj_t *HomeScreen::Create() {
  _subscription = ps_new_subscriber(10, PS_STRLIST("sensor.chamber.temperature"));
  lv_obj_t *screen = lv_obj_create(NULL);
  _objects->temperature = lv_label_create(screen);
  lv_obj_center(_objects->temperature);
  lv_label_set_text(_objects->temperature, "--°C");
  return screen;
}
void HomeScreen::Loop() {
  ps_msg_t *msg = nullptr;
  for ((msg = ps_get(_subscription, 0)); msg != NULL; (msg = ps_get(_subscription, 0))) {
    if (ps_has_topic(msg, "sensor.chamber.temperature") && PS_IS_INT(msg)) {
      FLOG_DEBUG("Received temperature: %d", (int)msg->int_val);
      float temperature = msg->int_val / 100.0f;
      char temp_str[16];
      snprintf(temp_str, sizeof(temp_str), "%.1f°C", temperature);
      lv_label_set_text(_objects->temperature, temp_str);
    }
  }
}
} // namespace toothless