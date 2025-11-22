#pragma once

#include <esp_err.h>

#include <memory>

#include "ui/screens/screen.hpp"

namespace toothless {

struct ErrorMessage {
  std::string title;
  std::string message;
  std::string id;  // TODO: needs to be an enum, and a way to clear messages.
};
class ErrorScreen : public Screen {
 public:
  ErrorScreen();
  ~ErrorScreen();

  lv_obj_t* Create() override;
  void Loop() override;
  // ScreenLabels* GetLabels() override { return _labels.get(); };

 private:
  std::vector<ErrorMessage> _errors;
};

}  // namespace toothless
