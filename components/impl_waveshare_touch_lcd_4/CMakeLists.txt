file(GLOB_RECURSE SRC "src/*.cpp" "src/*.c")
set(INCL "include")
set(PRIVREQ 
  esp_psram
  FunLog
  esp_lcd
  esp_lvgl_port
  esp_timer
  lvgl
  io_manager
  # esp_app_format
)
set(REQ 
)
# if (NOT DEFINED CONFIG_SPIRAM)
#   message(FATAL_ERROR "Waveshare Touch LCD 4 implementation requires SPIRAM to be enabled")
# endif()
if(CONFIG_IMPL_WAVESHARE_TOUCH_LCD_4)
  message(STATUS "[x] Impl: Waveshare Touch LCD 4 display implementation")
  set(CONFIG_IMPL_DISPLAY_HRES ${CONFIG_IMPL_WAVESHARE_TOUCH_LCD_4_HRES})
  set(CONFIG_IMPL_DISPLAY_VRES ${CONFIG_IMPL_WAVESHARE_TOUCH_LCD_4_VRES})
  idf_component_register(SRCS ${SRC} INCLUDE_DIRS ${INCL} REQUIRES ${REQ} PRIV_REQUIRES ${PRIVREQ})
else()
  idf_component_register(SRCS "" INCLUDE_DIRS "" REQUIRES "" PRIV_REQUIRES ${PRIVREQ})
endif()
# idf_component_register(SRC_DIRS "." INCLUDE_DIRS "include")
# target_include_directories(${COMPONENT_LIB} INTERFACE "include")
