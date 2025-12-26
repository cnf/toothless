/// @file widget_factories.cpp
/// @brief Pre-styled widget factory implementations
#include "widget_factories.hpp"

#include "style_registry.hpp"

namespace toothless::ui {

// ============================================================================
// BUTTON FACTORIES
// ============================================================================

lv_obj_t* CreatePrimaryButton(lv_obj_t* parent, const char* text, int32_t width, int32_t height, bool grow) {
  lv_obj_t* btn = lv_button_create(parent);
  lv_obj_add_style(btn, &themes::buttons::primary, 0);
  lv_obj_add_style(btn, &themes::buttons::pressed, LV_STATE_PRESSED);
  lv_obj_add_style(btn, &themes::buttons::disabled, LV_STATE_DISABLED);

  if (height != NULL) {
    lv_obj_set_height(btn, height);
  }
  lv_obj_set_width(btn, width);
  lv_obj_set_flex_grow(btn, grow);

  lv_obj_t* label = lv_label_create(btn);
  lv_label_set_text(label, text);
  lv_obj_center(label);

  return btn;
}

lv_obj_t* CreatePrimaryButton(lv_obj_t* parent, const char* text, bool grow) {
  return CreatePrimaryButton(parent, text, LV_SIZE_CONTENT, LV_SIZE_CONTENT, grow);
}

lv_obj_t* CreateSecondaryButton(lv_obj_t* parent, const char* text, int32_t width, int32_t height, bool grow) {
  lv_obj_t* btn = lv_button_create(parent);
  lv_obj_add_style(btn, &themes::buttons::secondary, 0);
  lv_obj_add_style(btn, &themes::buttons::pressed, LV_STATE_PRESSED);
  lv_obj_add_style(btn, &themes::buttons::disabled, LV_STATE_DISABLED);

  if (height != NULL) {
    lv_obj_set_height(btn, height);
  }
  lv_obj_set_width(btn, width);
  lv_obj_set_flex_grow(btn, grow);

  lv_obj_t* label = lv_label_create(btn);
  lv_label_set_text(label, text);
  lv_obj_center(label);

  return btn;
}
lv_obj_t* CreateSecondaryButton(lv_obj_t* parent, const char* text, bool grow) {
  return CreateSecondaryButton(parent, text, LV_SIZE_CONTENT, LV_SIZE_CONTENT, grow);
}

lv_obj_t* CreateDangerButton(lv_obj_t* parent, const char* text, int32_t width, int32_t height, bool grow) {
  lv_obj_t* btn = lv_button_create(parent);
  lv_obj_add_style(btn, &themes::buttons::danger, 0);
  lv_obj_add_style(btn, &themes::buttons::pressed, LV_STATE_PRESSED);
  lv_obj_add_style(btn, &themes::buttons::disabled, LV_STATE_DISABLED);

  if (height != NULL) {
    lv_obj_set_height(btn, height);
  }
  lv_obj_set_width(btn, width);
  lv_obj_set_flex_grow(btn, grow);

  lv_obj_t* label = lv_label_create(btn);
  lv_label_set_text(label, text);
  lv_obj_center(label);

  return btn;
}

lv_obj_t* CreateDangerButton(lv_obj_t* parent, const char* text, bool grow) {
  return CreateDangerButton(parent, text, LV_SIZE_CONTENT, LV_SIZE_CONTENT, grow);
}

lv_obj_t* CreateSuccessButton(lv_obj_t* parent, const char* text, int32_t width, int32_t height, bool grow) {
  lv_obj_t* btn = lv_button_create(parent);
  lv_obj_add_style(btn, &themes::buttons::success, 0);
  lv_obj_add_style(btn, &themes::buttons::pressed, LV_STATE_PRESSED);
  lv_obj_add_style(btn, &themes::buttons::disabled, LV_STATE_DISABLED);

  if (height != NULL) {
    lv_obj_set_height(btn, height);
  }
  lv_obj_set_width(btn, width);
  lv_obj_set_flex_grow(btn, grow);

  lv_obj_t* label = lv_label_create(btn);
  lv_label_set_text(label, text);
  lv_obj_center(label);

  return btn;
}

lv_obj_t* CreateSuccessButton(lv_obj_t* parent, const char* text, bool grow) {
  return CreateSuccessButton(parent, text, LV_SIZE_CONTENT, LV_SIZE_CONTENT, grow);
}

lv_obj_t* CreateSettingsButton(lv_obj_t* parent, int32_t width, int32_t height, bool grow) {
  lv_obj_t* btn = lv_button_create(parent);
  lv_obj_add_style(btn, &themes::buttons::settings, 0);
  lv_obj_add_style(btn, &themes::buttons::pressed, LV_STATE_PRESSED);

  if (height != NULL) {
    lv_obj_set_height(btn, height);
  }
  lv_obj_set_width(btn, width);
  lv_obj_set_flex_grow(btn, grow);

  lv_obj_t* label = lv_label_create(btn);
  lv_label_set_text(label, LV_SYMBOL_SETTINGS);
  lv_obj_center(label);

  return btn;
}

lv_obj_t* CreateSettingsButton(lv_obj_t* parent, bool grow) {
  lv_obj_t* btn = lv_button_create(parent);
  lv_obj_add_style(btn, &themes::buttons::settings, 0);
  lv_obj_add_style(btn, &themes::buttons::pressed, LV_STATE_PRESSED);

  // FIXME: make it grow on flag
  lv_obj_set_width(btn, lv_obj_get_height(btn));

  lv_obj_t* label = lv_label_create(btn);
  lv_label_set_text(label, LV_SYMBOL_SETTINGS);
  lv_obj_center(label);

  return btn;
}

// ============================================================================
// SCREEN & CONTAINER FACTORIES
// ============================================================================

lv_obj_t* CreateScreen() {
  lv_obj_t* screen = lv_obj_create(NULL);
  lv_obj_add_style(screen, &themes::screens::background, 0);
  lv_obj_remove_flag(screen, LV_OBJ_FLAG_SCROLLABLE);
  return screen;
}

lv_obj_t* CreateSubScreen(lv_obj_t* parent) {
  lv_obj_t* screen = lv_obj_create(parent);
  lv_obj_remove_flag(screen, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_remove_flag(screen, LV_OBJ_FLAG_FLEX_IN_NEW_TRACK);
  lv_obj_add_flag(screen, LV_OBJ_FLAG_FLOATING);
  lv_obj_add_style(screen, &themes::screens::subscreen, 0);
  lv_obj_set_size(screen, lv_pct(100), lv_pct(100));
  lv_obj_set_pos(screen, 0, 0);

  return screen;
}

lv_obj_t* CreateCard(lv_obj_t* parent) {
  lv_obj_t* card = lv_obj_create(parent);
  lv_obj_add_style(card, &themes::screens::card, 0);
  lv_obj_remove_flag(card, LV_OBJ_FLAG_SCROLLABLE);

  return card;
}

lv_obj_t* CreateContainer(lv_obj_t* parent) {
  lv_obj_t* container = lv_obj_create(parent);
  lv_obj_add_style(container, &themes::screens::container, 0);
  lv_obj_remove_flag(container, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_size(container, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
  // lv_obj_set_style_pad_all(container, 0, 0);
  return container;
}

lv_obj_t* CreateRowContainer(lv_obj_t* parent) {
  lv_obj_t* container = lv_obj_create(parent);
  lv_obj_add_style(container, &themes::screens::rowcontainer, 0);
  lv_obj_remove_flag(container, LV_OBJ_FLAG_SCROLLABLE);
  // lv_obj_set_style_pad_gap(container, 10, 0);
  return container;
}

lv_obj_t* CreateColumnContainer(lv_obj_t* parent) {
  lv_obj_t* container = lv_obj_create(parent);
  lv_obj_add_style(container, &themes::screens::columncontainer, 0);
  lv_obj_remove_flag(container, LV_OBJ_FLAG_SCROLLABLE);
  return container;
}

lv_obj_t* CreateMenuContainer(lv_obj_t* parent) {
  lv_obj_t* container = lv_menu_cont_create(parent);
  lv_obj_add_style(container, &themes::screens::menu_container, 0);
  lv_obj_add_style(container, &themes::screens::menu_selected, LV_PART_MAIN | LV_STATE_CHECKED);
  lv_obj_add_style(container, &themes::screens::menu_selected, LV_PART_MAIN | LV_STATE_FOCUSED);
  lv_obj_add_style(container, &themes::screens::menu_unselected, LV_PART_MAIN | LV_STATE_DEFAULT);
  return container;
}

lv_obj_t* CreateMenu(lv_obj_t* parent, const char* title) {
  lv_obj_t* menu = lv_menu_create(parent);
  lv_obj_add_style(menu, &themes::screens::menu, 0);
  lv_obj_set_style_flex_grow(menu, 1, LV_PART_MAIN);
  lv_obj_set_size(menu, lv_pct(100), lv_pct(100));

  lv_obj_t* header = lv_menu_get_main_header(menu);
  if (header) lv_obj_add_style(header, &themes::screens::menu_header, 0);
  lv_obj_t* header_label = lv_obj_get_child_by_type(header, 0, &lv_label_class);
  if (header_label) lv_obj_add_style(header_label, &themes::text::title, 0);

  if (title) {
    lv_label_set_text(header_label, title);
  }

  lv_menu_set_mode_root_back_button(menu, LV_MENU_ROOT_BACK_BUTTON_ENABLED);
  return menu;
}

lv_obj_t* CreateMenuPage(lv_obj_t* parent, const char* title) {
  lv_obj_t* page = lv_menu_page_create(parent, title);
  lv_obj_add_style(page, &themes::screens::menu_page, 0);

  return page;
}

lv_obj_t* CreateMenuSection(lv_obj_t* parent) {
  lv_obj_t* section = lv_menu_section_create(parent);
  lv_obj_add_style(section, &themes::screens::menu_section, 0);
  return section;
}

lv_obj_t* CreateMenuRootPage(lv_obj_t* parent, const char* title) {
  lv_obj_t* page = CreateMenuPage(parent, title);

  lv_menu_set_sidebar_page(parent, page);
  lv_obj_t* header = lv_menu_get_sidebar_header(parent);
  lv_obj_t* header_label = lv_obj_get_child_by_type(header, 0, &lv_label_class);
  if (header_label) lv_obj_add_style(header_label, &themes::text::title, 0);
  return page;
}

lv_obj_t* CreateMenuRootSection(lv_obj_t* parent) {
  lv_obj_t* section = lv_menu_section_create(parent);
  lv_obj_add_style(section, &themes::screens::menu_section, 0);
  return section;
}

lv_obj_t* CreateMenuRootEntry(lv_obj_t* parent, const char* title, const char* icon) {
  lv_obj_t* label = NULL;

  if (title) {
    label = lv_label_create(parent);
    lv_label_set_text(label, title);
    lv_label_set_long_mode(label, LV_LABEL_LONG_SCROLL_CIRCULAR);
    // lv_obj_set_flex_grow(label, 1);
  }
  return CreateMenuRootEntry(parent, label, icon);
}

lv_obj_t* CreateMenuRootEntry(lv_obj_t* parent, lv_obj_t* obj, const char* icon) {
  lv_obj_t* img = NULL;

  lv_obj_t* entry = lv_menu_cont_create(parent);
  lv_obj_add_style(entry, &themes::screens::menu_container, 0);
  lv_obj_add_style(entry, &themes::screens::menu_selected, LV_PART_MAIN | LV_STATE_CHECKED);
  lv_obj_add_style(entry, &themes::screens::menu_selected, LV_PART_MAIN | LV_STATE_FOCUSED);
  lv_obj_add_style(entry, &themes::screens::menu_unselected, LV_PART_MAIN | LV_STATE_DEFAULT);

  if (icon) {
    // CreateImage(entry, icon);
    img = lv_image_create(entry);
    lv_image_set_src(img, icon);
  }

  if (obj) {
    lv_obj_set_parent(obj, entry);
  }
  return entry;
}

// lv_obj_t* CreateMenuSeparator(lv_obj_t* parent) { lv_menu_separator_create(sub_page); }

lv_obj_t* StyleMenuSidebar(lv_obj_t* menu) {
  lv_obj_t* sidebar_header = lv_menu_get_sidebar_header(menu);
  lv_obj_t* sb_back = lv_menu_get_sidebar_header_back_button(menu);
  if (sidebar_header) {
    lv_obj_add_style(sidebar_header, &themes::screens::menu_header, 0);

    // lv_obj_set_style_pad_all(sidebar_header, 0, 0);
    lv_obj_t* header_label = lv_obj_get_child_by_type(sidebar_header, 0, &lv_label_class);
    if (header_label) {
      lv_obj_add_style(header_label, &themes::text::title, 0);
    }
    // lv_obj_t* sidebar = lv_menu_get_cur_sidebar_page(menu);
    // if (sidebar) lv_obj_add_style(sidebar, &themes::screens::sidebar, LV_PART_MAIN);
    // if (sidebar) lv_obj_set_width(sidebar, LV_SIZE_CONTENT);
    if (!sb_back) return sidebar_header;
    lv_obj_set_user_data(sidebar_header, sb_back);
    lv_obj_add_flag(sidebar_header, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(
        sidebar_header,
        [](lv_event_t* e) {
          lv_obj_t* target = static_cast<lv_obj_t*>(lv_event_get_target(e));
          lv_obj_t* back_btn = static_cast<lv_obj_t*>(lv_obj_get_user_data(target));
          lv_obj_send_event(back_btn, LV_EVENT_CLICKED, e);
        },
        LV_EVENT_CLICKED, NULL);
  }

  return sidebar_header;
}

// ============================================================================
// TEXT/LABEL FACTORIES
// ============================================================================

lv_obj_t* CreateTitle(lv_obj_t* parent, const char* text) {
  lv_obj_t* label = lv_label_create(parent);
  lv_label_set_text(label, text);
  lv_obj_add_style(label, &themes::text::title, 0);
  lv_obj_add_style(label, &themes::text::danger, LV_STATE_USER_1);
  lv_obj_add_style(label, &themes::text::warning, LV_STATE_USER_2);
  return label;
}

lv_obj_t* CreateHeading(lv_obj_t* parent, const char* text) {
  lv_obj_t* label = lv_label_create(parent);
  lv_label_set_text(label, text);
  lv_obj_add_style(label, &themes::text::heading, 0);
  lv_obj_add_style(label, &themes::text::danger, LV_STATE_USER_1);
  lv_obj_add_style(label, &themes::text::warning, LV_STATE_USER_2);
  return label;
}

lv_obj_t* CreateBodyText(lv_obj_t* parent, const char* text) {
  lv_obj_t* label = lv_label_create(parent);
  lv_obj_add_style(label, &themes::text::body, 0);
  lv_label_set_long_mode(label, LV_LABEL_LONG_MODE_WRAP);
  lv_obj_set_height(label, LV_SIZE_CONTENT);
  lv_label_set_text(label, text);
  return label;
}

lv_obj_t* CreateSmallText(lv_obj_t* parent, const char* text) {
  lv_obj_t* label = lv_label_create(parent);
  lv_label_set_text(label, text);
  lv_obj_add_style(label, &themes::text::small, 0);
  lv_obj_add_style(label, &themes::text::danger, LV_STATE_USER_1);
  lv_obj_add_style(label, &themes::text::warning, LV_STATE_USER_2);
  return label;
}

lv_obj_t* CreateValueLarge(lv_obj_t* parent, float value, const char* format) {
  lv_obj_t* label = lv_label_create(parent);
  lv_obj_add_style(label, &themes::text::value_large, 0);
  lv_obj_add_style(label, &themes::text::danger, LV_STATE_USER_1);
  lv_obj_add_style(label, &themes::text::warning, LV_STATE_USER_2);

  // UpdateValueLabel(label, value, format);
  lv_label_set_text_fmt(label, format, value);

  lv_obj_set_width(label, LV_SIZE_CONTENT);
  lv_obj_set_height(label, LV_SIZE_CONTENT);
  return label;
}

lv_obj_t* CreateValueSmall(lv_obj_t* parent, float value, const char* format) {
  lv_obj_t* label = lv_label_create(parent);
  lv_obj_add_style(label, &themes::text::value_small, 0);
  lv_obj_add_style(label, &themes::text::danger, LV_STATE_USER_1);
  lv_obj_add_style(label, &themes::text::warning, LV_STATE_USER_2);

  lv_label_set_text_fmt(label, format, value);
  lv_obj_set_width(label, LV_SIZE_CONTENT);
  lv_obj_set_height(label, LV_SIZE_CONTENT);
  return label;
}

lv_obj_t* CreateUnitLabel(lv_obj_t* parent, const char* unit) {
  lv_obj_t* label = lv_label_create(parent);
  lv_label_set_text(label, unit);
  lv_obj_add_style(label, &themes::text::unit, 0);
  return label;
}

lv_obj_t* CreateTextArea(lv_obj_t* parent) {
  lv_obj_t* area = lv_textarea_create(parent);
  lv_obj_add_style(area, &themes::text::textentry, 0);
  return area;
}

lv_obj_t* CreateTextLine(lv_obj_t* parent) {
  lv_obj_t* line = CreateTextArea(parent);
  lv_textarea_set_one_line(line, true);
  lv_textarea_set_align(line, LV_TEXT_ALIGN_CENTER);
  lv_obj_set_height(line, LV_SIZE_CONTENT);
  return line;
}

lv_obj_t* CreateIconItem(lv_obj_t* parent, const char* txt, const char* icon) {
  lv_obj_t* obj = CreateRowContainer(parent);
  lv_obj_add_style(obj, &themes::text::body, 0);
  lv_obj_set_flex_align(obj, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
  // lv_obj_set_width(obj, lv_pct(100));
  lv_obj_set_size(obj, lv_pct(100), LV_SIZE_CONTENT);

  lv_obj_t* img = NULL;
  lv_obj_t* label = NULL;

  if (icon) {
    img = lv_image_create(obj);
    lv_image_set_src(img, icon);
    lv_obj_add_style(img, &themes::text::body, 0);
  }

  if (txt) {
    label = CreateBodyText(obj, txt);
    lv_obj_set_height(label, LV_SIZE_CONTENT);
    // lv_label_set_long_mode(label, LV_LABEL_LONG_MODE_WRAP);
    lv_obj_set_width(label, lv_pct(100));
  }
  return obj;
}

// ============================================================================
// CONTROL FACTORIES
// ============================================================================

lv_obj_t* CreateSlider(lv_obj_t* parent, int32_t min, int32_t max, int32_t value) {
  lv_obj_t* slider = lv_slider_create(parent);
  lv_obj_add_style(slider, &themes::controls::slider_main, LV_PART_MAIN);
  lv_obj_add_style(slider, &themes::controls::slider_indicator, LV_PART_INDICATOR);
  lv_obj_add_style(slider, &themes::controls::slider_knob, LV_PART_KNOB);

  lv_slider_set_range(slider, min, max);
  lv_slider_set_value(slider, value, LV_ANIM_OFF);

  return slider;
}

lv_obj_t* CreateSwitch(lv_obj_t* parent, bool initial_state) {
  lv_obj_t* sw = lv_switch_create(parent);
  lv_obj_add_style(sw, &themes::controls::switch_bg, LV_PART_MAIN);
  lv_obj_add_style(sw, &themes::controls::switch_indicator, LV_PART_INDICATOR | LV_STATE_CHECKED);
  lv_obj_add_style(sw, &themes::controls::switch_knob, LV_PART_KNOB);

  if (initial_state) {
    lv_obj_add_state(sw, LV_STATE_CHECKED);
  }

  return sw;
}

lv_obj_t* CreateRoller(lv_obj_t* parent, const char* options, int32_t selected) {
  lv_obj_t* roller = lv_roller_create(parent);
  lv_obj_add_style(roller, &themes::controls::roller, 0);
  lv_obj_add_style(roller, &themes::controls::roller_selected, LV_PART_SELECTED);
  lv_roller_set_visible_row_count(roller, 4);
  lv_obj_set_style_min_height(roller, LV_DPX(80), 0);
  lv_obj_set_style_min_width(roller, LV_DPX(40), 0);
  lv_obj_set_width(roller, LV_SIZE_CONTENT);

  lv_roller_set_options(roller, options, LV_ROLLER_MODE_NORMAL);
  lv_roller_set_selected(roller, selected, LV_ANIM_OFF);

  return roller;
}

lv_obj_t* CreateSmallRoller(lv_obj_t* parent, const char* options, int32_t selected) {
  lv_obj_t* roller = CreateRoller(parent, options, selected);
  lv_obj_set_style_text_font(roller, &themes::fonts::small, 0);
  lv_roller_set_visible_row_count(roller, 3);

  return roller;
}

// static void dropdown_opened_cb(lv_event_t* e) {
//   lv_obj_t* dd = lv_event_get_target_obj(e);
//   lv_obj_t* list = lv_dropdown_get_list(dd);

//   if (list) {
//     // Now we can configure the list
//     lv_obj_add_style(list, &themes::controls::dropdown_list, 0);
//     lv_obj_add_style(list, &themes::controls::dropdown_selected, LV_PART_SELECTED);
//     lv_obj_set_scroll_dir(list, LV_DIR_VER);
//     lv_obj_set_scroll_snap_y(list, LV_SCROLL_SNAP_NONE);
//     lv_obj_remove_flag(list, LV_OBJ_FLAG_SCROLL_MOMENTUM);
//     lv_obj_set_scrollbar_mode(list, LV_SCROLLBAR_MODE_AUTO);

//     // Critical: Ensure the list allows scrolling
//     lv_obj_add_flag(list, LV_OBJ_FLAG_SCROLLABLE);

//     // Set a reasonable height to trigger scrolling
//     // Adjust based on your item count
//     lv_obj_set_style_max_height(list, lv_pct(50), 0);
//   }
// }

lv_obj_t* CreateDropdown(lv_obj_t* parent, const char* name, const char* options, int32_t selected) {
  lv_obj_t* dd = lv_dropdown_create(parent);
  lv_obj_add_style(dd, &themes::controls::dropdown, 0);
  lv_obj_add_style(dd, &themes::controls::dropdown_indicator, LV_PART_INDICATOR);
  // lv_obj_add_style(dd, &themes::controls::dropdown_selected, LV_PART_SELECTED);
  lv_dropdown_set_options(dd, options);
  lv_dropdown_set_selected(dd, selected);
  lv_dropdown_set_text(dd, name);
  lv_dropdown_set_dir(dd, LV_DIR_LEFT);

  lv_dropdown_set_selected_highlight(dd, false);  // Optional: disable highlight
  lv_obj_set_style_text_decor(dd, LV_TEXT_DECOR_NONE, LV_PART_MAIN);

  // lv_obj_add_event_cb(dd, dropdown_opened_cb, LV_EVENT_READY, nullptr);

  lv_obj_t* list = lv_dropdown_get_list(dd);
  lv_obj_add_style(list, &themes::controls::dropdown_list, 0);
  lv_obj_add_style(list, &themes::controls::dropdown_selected, LV_PART_SELECTED);
  lv_obj_set_scroll_dir(list, LV_DIR_VER);
  lv_obj_set_scroll_snap_y(list, LV_SCROLL_SNAP_NONE);
  lv_obj_remove_flag(list, LV_OBJ_FLAG_SCROLL_MOMENTUM);
  lv_obj_set_scrollbar_mode(list, LV_SCROLLBAR_MODE_AUTO);

  return dd;
}

lv_obj_t* CreateButtonMatrix(lv_obj_t* parent, const char* btn_map[], lv_event_cb_t event_cb, void* user_data) {
  lv_obj_t* btnm = lv_buttonmatrix_create(parent);
  lv_obj_add_style(btnm, &themes::controls::keypad, 0);
  lv_obj_add_style(btnm, &themes::controls::keys, LV_PART_ITEMS);
  lv_obj_add_style(btnm, &themes::controls::keys_pressed, LV_PART_ITEMS | LV_STATE_PRESSED);
  lv_obj_set_layout(btnm, LV_LAYOUT_GRID);
  lv_buttonmatrix_set_map(btnm, btn_map);
  if (event_cb) {
    lv_obj_add_event_cb(btnm, event_cb, LV_EVENT_VALUE_CHANGED, user_data);
  }
  return btnm;
}

// ============================================================================
// CHART FACTORIES
// ============================================================================

lv_obj_t* CreateChart(lv_obj_t* parent, uint16_t points_capacity) {
  lv_obj_t* chart = lv_chart_create(parent);
  lv_obj_add_style(chart, &themes::charts::background, 0);
  lv_obj_add_style(chart, &themes::charts::grid, LV_PART_MAIN);

  lv_chart_set_type(chart, LV_CHART_TYPE_LINE);
  lv_chart_set_point_count(chart, points_capacity);
  lv_chart_set_update_mode(chart, LV_CHART_UPDATE_MODE_SHIFT);
  lv_chart_set_div_line_count(chart, 5, 10);

  lv_obj_add_style(chart, &themes::charts::line_temp, LV_PART_ITEMS);
  lv_obj_add_style(chart, &themes::charts::indicator, LV_PART_INDICATOR);
  lv_obj_add_style(chart, &themes::charts::cursor, LV_PART_CURSOR);

  return chart;
}

lv_obj_t* CreateChartScale(lv_obj_t* parent, size_t tick_count, bool left) {
  lv_obj_t* scale = lv_scale_create(parent);
  if (left) {
    lv_scale_set_mode(scale, LV_SCALE_MODE_VERTICAL_LEFT);
  } else {
    lv_scale_set_mode(scale, LV_SCALE_MODE_VERTICAL_RIGHT);
  }
  lv_obj_add_style(scale, &themes::charts::scale, 0);
  lv_scale_set_total_tick_count(scale, tick_count);
  lv_scale_set_major_tick_every(scale, 1);
  lv_obj_set_size(scale, 40, lv_pct(100));  // FIXME: scale needs dynamic width based on font size
  lv_obj_set_flex_grow(scale, 0);           // Don't grow

  return scale;
}

// ============================================================================
// INDICATOR FACTORIES
// ============================================================================

lv_obj_t* CreateLEDIndicator(lv_obj_t* parent, bool initial_state) {
  lv_obj_t* led = lv_led_create(parent);

  // need to set color with lv_led_color_set(), the theme doesn't actually change the color.
  lv_led_set_brightness(led, LV_LED_BRIGHT_MAX);
  lv_led_set_color(led, themes::GetColors().danger);

  if (initial_state) {
    lv_obj_add_style(led, &themes::indicators::led_on, 0);
    lv_led_on(led);
  } else {
    lv_obj_add_style(led, &themes::indicators::led_off, 0);
    lv_led_off(led);
  }

  return led;
}

// lv_obj_t* CreateMessageBox(lv_obj_t* parent, std::string title, std::string message, std::string confirm_text,
//                            std::string cancel_text, std::function<void(void*)> on_confirm,
//                            std::function<void(void*)> on_cancel, void* user_data) {
lv_obj_t* CreateMessageBox(lv_obj_t* parent, std::string title, std::string message, std::string confirm_text,
                           std::string cancel_text, lv_event_cb_t on_confirm, lv_event_cb_t on_cancel,
                           void* user_data) {
  lv_obj_t* msgbox = lv_msgbox_create(parent);
  lv_obj_add_style(msgbox, &themes::menus::messagebox, 0);
  lv_obj_add_style(msgbox, &themes::menus::messagebox_backdrop, LV_PART_MAIN | LV_STATE_DEFAULT);

  lv_obj_set_size(msgbox, lv_pct(90), lv_pct(90));
  // lv_obj_set_flex_align(msgbox, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

  lv_obj_move_to_index(msgbox, -1);
  lv_obj_set_style_text_align(msgbox, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);

  if (!message.empty()) lv_msgbox_add_text(msgbox, message.c_str());
  if (!title.empty()) lv_msgbox_add_title(msgbox, title.c_str());

  if (cancel_text != "") {
    lv_obj_t* cancel_button = lv_msgbox_add_footer_button(msgbox, cancel_text.c_str());
    lv_obj_add_style(cancel_button, &themes::buttons::primary, 0);
    lv_obj_set_width(cancel_button, lv_pct(45));
    // lv_obj_set_height(cancel_button, 100);
    // lv_obj_set_flex_grow(cancel_button, 1);
    if (on_cancel) lv_obj_add_event_cb(cancel_button, on_cancel, LV_EVENT_CLICKED, user_data);
  }

  if (confirm_text != "") {
    lv_obj_t* confirm_button = lv_msgbox_add_footer_button(msgbox, confirm_text.c_str());
    lv_obj_add_style(confirm_button, &themes::buttons::primary, 0);
    lv_obj_set_width(confirm_button, lv_pct(45));
    // lv_obj_set_height(confirm_button, 100);
    // lv_obj_set_flex_grow(confirm_button, 1);
    if (on_confirm) lv_obj_add_event_cb(confirm_button, on_confirm, LV_EVENT_CLICKED, user_data);
  }
  lv_obj_t* footer = lv_msgbox_get_footer(msgbox);
  lv_obj_set_height(footer, LV_SIZE_CONTENT);
  lv_obj_t* header = lv_msgbox_get_header(msgbox);
  lv_obj_set_height(header, LV_SIZE_CONTENT);
  return msgbox;
}

// ============================================================================
// WIDGET UTILITY FUNCTIONS
// ============================================================================

lv_obj_t* CreateLabeledUnit(lv_obj_t* parent, const char* label_text, float value, const char* value_format,
                            const char* unit_text, bool small) {
  lv_obj_t* container = CreateColumnContainer(parent);
  lv_obj_set_width(container, LV_SIZE_CONTENT);
  lv_obj_set_style_pad_gap(container, 2, 0);

  // lv_obj_set_align(container, LV_ALIGN_CENTER);
  lv_obj_set_flex_align(container, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  lv_obj_set_flex_grow(container, 1);

  lv_obj_t* label = CreateSmallText(container, label_text);
  lv_obj_set_height(label, LV_SIZE_CONTENT);
  lv_obj_t* obj = CreateRowContainer(container);
  lv_obj_set_style_pad_gap(obj, 0, 0);

  lv_obj_set_height(obj, LV_SIZE_CONTENT);
  lv_obj_set_width(obj, LV_SIZE_CONTENT);
  lv_obj_set_flex_align(obj, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER);
  lv_obj_t* value_label;
  if (!small) value_label = CreateValueLarge(obj, value, value_format);
  if (small) value_label = CreateValueSmall(obj, value, value_format);
  lv_obj_set_style_pad_right(value_label, 0, 0);
  lv_obj_set_user_data(value_label, container);
  lv_obj_set_width(value_label, LV_SIZE_CONTENT);
  lv_obj_t* unit_label = CreateUnitLabel(obj, unit_text);
  lv_obj_set_style_pad_left(unit_label, 0, 0);
  lv_obj_set_width(unit_label, LV_SIZE_CONTENT);

  return value_label;
}

lv_obj_t* CreateLabeledFloatUnit(lv_obj_t* parent, const char* label_text, const char* unit_text, lv_subject_t* subject,
                                 const char* value_format, bool small) {
  lv_obj_t* label = CreateLabeledUnit(parent, label_text, -100, value_format, unit_text, small);
  lv_label_bind_text(label, subject, value_format);
  lv_obj_t* container = static_cast<lv_obj_t*>(lv_obj_get_user_data(label));
  lv_observer_t* observer = lv_subject_add_observer_obj(subject, ValueFloatObserverHandler, label, (void*)value_format);

  return label;
}

lv_obj_t* CreateLabeledIntUnit(lv_obj_t* parent, const char* label_text, const char* unit_text, lv_subject_t* subject,
                               const char* value_format, bool small) {
  lv_obj_t* label = CreateLabeledUnit(parent, label_text, -100, value_format, unit_text, small);
  lv_label_bind_text(label, subject, value_format);
  lv_obj_t* container = static_cast<lv_obj_t*>(lv_obj_get_user_data(label));
  lv_obj_bind_flag_if_lt(container, subject, LV_OBJ_FLAG_HIDDEN, -99);
  return label;
}
// lv_obj_t* CreateLabeledUnit(lv_obj_t* parent, const char* label_text, float value, const char* value_format,
//                             const char* unit_text) {}

// ============================================================================
// HELPER FUNCTIONS
// ============================================================================

void ValueFloatObserverHandler(lv_observer_t* observer, lv_subject_t* subject) {
  lv_obj_t* label = static_cast<lv_obj_t*>(lv_observer_get_target(observer));
  float value = lv_subject_get_float(subject);
  // const char* format = static_cast<const char*>(lv_observer_get_user_data(observer));
  // lv_label_set_text_fmt(label, format, value);
  lv_obj_t* container = static_cast<lv_obj_t*>(lv_obj_get_user_data(label));
  if (value < -99) {
    lv_obj_add_flag(container, LV_OBJ_FLAG_HIDDEN);
  } else {
    lv_obj_remove_flag(container, LV_OBJ_FLAG_HIDDEN);
  }
}

void DeleteObject(lv_obj_t* obj) {
  if (obj) lv_obj_delete(obj);
}

// void UpdateValueLabel(lv_obj_t* label, float value, const char* format) {
//   // BUG: this doesn't work, for some reason the label text doesn't update properly
//   if (label == NULL) return;
//   lv_label_set_text_fmt(label, format, value);
// }

void SetLEDState(lv_obj_t* led, bool on) {
  if (led == NULL) return;

  if (on) {
    lv_obj_remove_style(led, &themes::indicators::led_off, 0);
    lv_obj_add_style(led, &themes::indicators::led_on, 0);
    lv_led_on(led);
  } else {
    lv_obj_remove_style(led, &themes::indicators::led_on, 0);
    lv_obj_add_style(led, &themes::indicators::led_off, 0);
    lv_led_off(led);
  }
}

std::string SnakeToTitle(const std::string& snake_case) {
  std::string result;
  std::string word;

  auto flush_word = [&]() {
    if (!word.empty()) {
      if (word.length() <= 2) {
        // Capitalize entire word if 2 chars or less
        for (char c : word) result += std::toupper(c);
      } else {
        // Normal title case
        result += std::toupper(word[0]);
        result += word.substr(1);
      }
      word.clear();
    }
  };

  for (char c : snake_case) {
    if (c == '_') {
      flush_word();
      result += ' ';
    } else {
      word += c;
    }
  }
  flush_word();  // Don't forget last word

  return result;
}

std::string TitleToSnake(const std::string& title_case) {
  FLOG_ERROR("TitleToSnake is deprecated, please migrate to helpers::TitleToSnake");
  std::string result;

  for (size_t i = 0; i < title_case.length(); i++) {
    char c = title_case[i];

    if (c == ' ') {
      result += '_';
    } else if (std::isupper(c)) {
      // Add underscore before uppercase if not first char and prev wasn't underscore
      if (i > 0 && result.back() != '_') {
        result += '_';
      }
      result += std::tolower(c);
    } else {
      result += c;
    }
  }

  return result;
}

}  // namespace toothless::ui
