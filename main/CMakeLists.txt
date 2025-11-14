file(GLOB_RECURSE SRC 
  "main.cpp"
  "heater/*.cpp"
  "sensors/*.cpp"
  "ui/user_interface.cpp"
  "ui/chart_history.cpp"
  "ui/display/*.cpp"
  "ui/fonts/*.c"
  "ui/screens/screen_helpers.cpp"
  )

set(INCL "${CMAKE_SOURCE_DIR}/include" "${CMAKE_SOURCE_DIR}/main" "${INCLUDE_DIRECTORIES}")
set(PRIVREQ  
  esp_wifi
  esp_lcd
  esp_lcd_touch
  esp_lvgl_port
  lvgl
  esp_driver_spi
  esp_timer
  esp_app_format

  impl_waveshare_pi
  impl_lilygo_t_display_s3_long

  io_manager
  # lvgl
)

set(REQ 
  # pubsub-c
  # QDispatch
  FunLog
  # max6675
)

# if(CONFIG_TL_DISPLAY_COMP_WS_35_RPI_G)
#     set(SCREEN_DIR "480x320")
# elseif(CONFIG_TL_DISPLAY_COMP_LG_TDISPLAY_S3_LONG)
#     set(SCREEN_DIR "160x640")
# elseif(CONFIG_TL_DISPLAY_COMP_ROUND)
#     set(SCREEN_DIR "round")
# endif()

# list(APPEND INCL "${CMAKE_SOURCE_DIR}/main/ui/screens/${SCREEN_DIR}")

file(GLOB_RECURSE SCREENS "ui/screens/${SCREEN_DIR}/*.cpp")
list(APPEND SRC ${SCREENS})

idf_component_register(SRCS ${SRC} ${UI} INCLUDE_DIRS ${INCL} REQUIRES ${REQ} PRIV_REQUIRES ${PRIVREQ})

## Use std23
target_compile_features(${COMPONENT_LIB} PUBLIC cxx_std_23)
set_property(TARGET ${COMPONENT_LIB} PROPERTY CXX_STANDARD 23)

# Add compiler warnings
target_compile_options(${COMPONENT_LIB} PRIVATE
    -Wall
    -Wextra
    -Wreturn-type
    -Werror=return-type    # This makes it an error, not just warning
    -Wmissing-declarations
    -Wunused-variable
)


message(STATUS "main CMAKE_CXX_FLAGS_RELEASE: ${CMAKE_CXX_FLAGS_RELEASE}")
message(STATUS "main CMAKE_CXX_FLAGS: ${CMAKE_CXX_FLAGS}")
message(STATUS "main Compile options: ${COMPILE_OPTIONS}")
message(STATUS "main Compile definitions: ${COMPILE_DEFINITIONS}")
message(STATUS "main Link options: ${LINK_OPTIONS}")

