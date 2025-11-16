file(GLOB_RECURSE SRC "src/*.cpp" "src/*.c")

set(INCL "include")
set(PRIVREQ 
  FunLog
  esp_lcd
  esp_lvgl_port
  esp_driver_spi
  esp_timer
  lvgl
  # esp_app_format
)
set(REQ 
)
if(CONFIG_IMPL_WAVESHARE_35_RPI_G)
  message(STATUS "[x] Waveshare 3.5inch RPi (G) display implementation")
  set(CONFIG_IMPL_DISPLAY_HRES ${CONFIG_IMPL_WSRPIG_HRES})
  set(CONFIG_IMPL_DISPLAY_VRES ${CONFIG_IMPL_WSRPIG_VRES})
  idf_component_register(SRCS ${SRC} INCLUDE_DIRS ${INCL} REQUIRES ${REQ} PRIV_REQUIRES ${PRIVREQ})
else()
  idf_component_register(SRCS "" INCLUDE_DIRS "" REQUIRES "" PRIV_REQUIRES ${PRIVREQ})
endif()
# message(STATUS "WaveShare: 3.5inch RPi LCD (G)")

# idf_component_register(SRC_DIRS "." INCLUDE_DIRS "include")
# target_include_directories(${COMPONENT_LIB} INTERFACE "include")
