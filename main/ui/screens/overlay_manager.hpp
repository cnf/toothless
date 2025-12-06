#pragma once

#include <lvgl.h>

#include <functional>
#include <type_traits>
#include <utility>

#include "ui/display/display.hpp"
#include "ui/themes/widget_factories.hpp"

namespace toothless {
/// @brief Manage overlay lifetime
///
/// Usage:
/// ```cpp
///  auto& mgr = OverlayManager::Instance();
/// if (mgr.IsActive()) return;
///
/// auto* state = mgr.Open<NumpadState>(ctx.parent_screen);
/// state->on_confirm = ...;
/// ```
/// Destroy with:
/// ```cpp
/// OverlayManager::Instance().Close();
/// ```
class OverlayManager {
 public:
  static OverlayManager& Instance() {
    static OverlayManager instance;
    return instance;
  }

  /// @brief Open overlay, returns state ptr. Closes any existing overlay first.
  /// @tparam State Type of overlay state
  /// @param parent Parent LVGL object
  /// @return Pointer to overlay state
  template <typename State>
  State* Open(lv_obj_t* parent) {
    Close();  // Close existing if any

    auto* state = new State();
    _backdrop = ui::CreateSubScreen(parent);
    _deleter = [state]() { delete state; };

    lv_obj_set_user_data(_backdrop, this);
    lv_obj_add_event_cb(_backdrop, DeleteCb, LV_EVENT_DELETE, nullptr);
    lv_obj_set_size(_backdrop, lv_pct(100), lv_pct(100));
    lv_obj_move_to_index(_backdrop, -1);

    state->backdrop = _backdrop;
    _active = true;
    return state;
  }

  /// @brief Close current overlay (async-safe)
  void Close() {
    if (_backdrop) {
      lv_obj_delete_async(_backdrop);
      // DeleteCb will handle cleanup
    }
  }

  /// @brief Check if overlay is active
  /// @return True if active
  bool IsActive() const { return _active; }

  /// @brief Get backdrop object
  /// @return Backdrop LVGL object
  lv_obj_t* Backdrop() const { return _backdrop; }

 private:
  lv_obj_t* _backdrop = nullptr;
  std::function<void()> _deleter;
  bool _active = false;

  OverlayManager() = default;  //<! Prevent external construction

  static void DeleteCb(lv_event_t* e) {
    auto* mgr = static_cast<OverlayManager*>(lv_obj_get_user_data(lv_event_get_target_obj(e)));
    if (mgr) {
      if (mgr->_deleter) mgr->_deleter();
      mgr->_deleter = nullptr;
      mgr->_backdrop = nullptr;
      mgr->_active = false;
    }
  }
};

}  // namespace toothless