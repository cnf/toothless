# set(SRC funlog_write.c)
file(GLOB_RECURSE SRC "src/*.cpp" "src/*.c")

# set(INCL "${CMAKE_SOURCE_DIR}/include") # "${INCLUDE_DIRECTORIES}")
if(CONFIG_TL_DISPLAY_COMP_WS_35_RPI_G)
  message(STATUS "[x] Waveshare 3.5inch RPi (G) display implementation")
  set(INCL "include")
  set(SOURCES ${SRC})
else()
  message(STATUS "[ ] Waveshare 3.5inch RPi (G) display implementation")
  set(INCL "")
  set(SOURCES "")
endif()
# set(INCL "include")
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
idf_component_register(SRCS ${SRC} INCLUDE_DIRS ${INCL} REQUIRES ${REQ} PRIV_REQUIRES ${PRIVREQ})
# message(STATUS "WaveShare: 3.5inch RPi LCD (G)")

# idf_component_register(SRC_DIRS "." INCLUDE_DIRS "include")
# target_include_directories(${COMPONENT_LIB} INTERFACE "include")
