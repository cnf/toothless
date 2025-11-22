file(GLOB_RECURSE SRC 
  "main.cpp"
  "heater/*.cpp"
  "sensors/*.cpp"
  "ui/user_interface.cpp"
  "ui/chart_history.cpp"
  "ui/display/*.cpp"
  "ui/fonts/*.c"
  "ui/themes/*.cpp"
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
  impl_cyb_4_3inch
  impl_lilygo_t_hmi
  
  io_manager
  # lvgl
)

set(REQ 
  # pubsub-c
  # QDispatch
  FunLog
  # max6675
)

if(CONFIG_IMPL_COMP_WS_35_RPI_G)
    set(SCREEN_DIR "480x320")
elseif(CONFIG_IMPL_LILYGO_TDISPLAY_S3_LONG)
    set(SCREEN_DIR "180x640")
elseif(CONFIG_IMPL_CYB_4_3_INCH)
    # set(SCREEN_DIR "180x640") # TODO: just for developing until proper screen is available
    set(SCREEN_DIR "rectangular") 
    # set(SCREEN_DIR "800x480")
elseif(CONFIG_IMPL_LILYGO_T_HMI)
    set(SCREEN_DIR "rectangular") 
endif()

list(APPEND INCL "${CMAKE_SOURCE_DIR}/main/ui/screens/${SCREEN_DIR}")

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
    -Wno-narrowing
)


message(STATUS "main CMAKE_CXX_FLAGS_RELEASE: ${CMAKE_CXX_FLAGS_RELEASE}")
message(STATUS "main CMAKE_CXX_FLAGS: ${CMAKE_CXX_FLAGS}")
message(STATUS "main Compile options: ${COMPILE_OPTIONS}")
message(STATUS "main Compile definitions: ${COMPILE_DEFINITIONS}")
message(STATUS "main Link options: ${LINK_OPTIONS}")

