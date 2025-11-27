file(GLOB_RECURSE SRC "src/*.cpp" "src/*.c")
set(INCL "include")
set(PRIVREQ 
  esp_psram
  FunLog
  esp_lcd
  esp_lvgl_port
  esp_timer
  lvgl
  # esp_app_format
  io_manager
)
set(REQ 
)

if(CONFIG_IMPL_CYB_4_3_INCH)
  message(STATUS "[x] Impl: Cheap Yellow Board 4.3\" display implementation")
  set(CONFIG_IMPL_DISPLAY_HRES ${CONFIG_IMPL_CYB_4_3INCH_HRES})
  set(CONFIG_IMPL_DISPLAY_VRES ${CONFIG_IMPL_CYB_4_3INCH_VRES})
  idf_component_register(SRCS ${SRC} INCLUDE_DIRS ${INCL} REQUIRES ${REQ} PRIV_REQUIRES ${PRIVREQ})
else()
  idf_component_register(SRCS "" INCLUDE_DIRS "" REQUIRES "" PRIV_REQUIRES ${PRIVREQ})
endif()
# idf_component_register(SRC_DIRS "." INCLUDE_DIRS "include")
# target_include_directories(${COMPONENT_LIB} INTERFACE "include")
