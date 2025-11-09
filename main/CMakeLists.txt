file(GLOB_RECURSE SRC 
  "main.cpp"
  "*.cpp"
  "ui/display/theme.cpp"

  )

set(INCL "${CMAKE_SOURCE_DIR}/include" "${CMAKE_SOURCE_DIR}/main" "${INCLUDE_DIRECTORIES}")
set(PRIVREQ  
  esp_wifi
  esp_lcd
  esp_lvgl_port
  esp_driver_spi
  esp_timer
  esp_app_format

  io_manager
  # lvgl
)

set(REQ 
  # pubsub-c
  # QDispatch
  FunLog
  # max6675
)

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

