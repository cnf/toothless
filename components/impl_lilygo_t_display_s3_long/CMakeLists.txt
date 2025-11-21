file(GLOB_RECURSE SRC "src/*.cpp" "src/*.c")
set(INCL "include")
set(PRIVREQ
  esp_timer
  esp_lcd
  esp_lvgl_port
  esp_driver_spi
  esp_mm
  lvgl
  # esp_app_format
  io_manager
  FunLog
)
set(REQ 
)
if(CONFIG_IMPL_LILYGO_TDISPLAY_S3_LONG)
  message(STATUS "[x] LilyGo T-Display S3 Long display implementation")
  set(CONFIG_IMPL_DISPLAY_HRES ${CONFIG_IMPL_LILYGO_TDISPLAY_S3_LONG_HRES})
  set(CONFIG_IMPL_DISPLAY_VRES ${CONFIG_IMPL_LILYGO_TDISPLAY_S3_LONG_VRES})
    # ... map all IMPL_LILYGO_* to IMPL_*
  idf_component_register(SRCS ${SRC} INCLUDE_DIRS ${INCL} REQUIRES ${REQ} PRIV_REQUIRES ${PRIVREQ})
else()
  idf_component_register(SRCS "" INCLUDE_DIRS "" REQUIRES "" PRIV_REQUIRES ${PRIVREQ})
endif()

# target_compile_options(${COMPONENT_LIB} PRIVATE
#     # -Wall
#     # -Wextra
#     -Wreturn-type
#     -Werror=return-type    # This makes it an error, not just warning
#     -Wmissing-declarations
#     -Wunused-variable
#     -Wno-narrowing
# )

# idf_component_register(SRC_DIRS "." INCLUDE_DIRS "include")
# target_include_directories(${COMPONENT_LIB} INTERFACE "include")
