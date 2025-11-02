#pragma once

#include "ui/screens/screen.hpp"

#include <memory>

namespace toothless {

struct HomeScreenLabels {
  lv_obj_t *temperature;
};

class HomeScreen : public Screen {
public:
  HomeScreen();
  lv_obj_t *Create();
  void Loop();

private:
  std::unique_ptr<HomeScreenLabels> _objects;
  // ps_subscriber_t *_subscription;
};
} // namespace toothless