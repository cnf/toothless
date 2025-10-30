file(GLOB_RECURSE SRC 
  "main.cpp"
  "*.cpp"

  )

# if(CONFIG_HAS_DISPLAY)
#   message(STATUS "[x] Setting UI source files")
#   if(CONFIG_USE_SSD1306_32 OR CONFIG_USE_SSD1306_64)
#     file(GLOB_RECURSE UI "ui/128_32/*.c")
#     message(STATUS "[x] Using SSD1306 128x32 display")
#   elseif(CONFIG_USE_M5_DISPLAY)
#     file(GLOB_RECURSE UI "ui/320_240/*.c")
#     message(STATUS "[x] Using M5 320x240 display")
#   endif()
# endif()

set(INCL "${CMAKE_SOURCE_DIR}/include" "${CMAKE_SOURCE_DIR}/main" "${INCLUDE_DIRECTORIES}")
set(PRIVREQ  
  esp_wifi
  esp_lcd
  esp_lvgl_port
  # lvgl
)

set(REQ 
  # pubsub-c
  # QDispatch
  FunLog
)

# if(CONFIG_IS_GATEWAY)
#   message(STATUS "[x] Building with Gateway component")
#   list(APPEND PRIVREQ gateway)     # Add gateway to the required components, REQ here is a copy from the parent scope
#   # set(REQ ${REQ} PARENT_SCOPE) # Cmake fuckery, we need to set this in the parent scope
# else()
#   message(STATUS "[ ] Building without Gateway component")
# endif()
# message(STATUS "REQUIRES components: ${REQ}")
# message(STATUS "PRIV_REQUIRES components: ${PRIVREQ}")
# set(REQ ${REQ})
# set(PRIVREQ ${PRIVREQ})

idf_component_register(SRCS ${SRC} ${UI} INCLUDE_DIRS ${INCL} REQUIRES ${REQ} PRIV_REQUIRES ${PRIVREQ})

## Use std23
target_compile_features(${COMPONENT_LIB} PUBLIC cxx_std_23)
set_property(TARGET ${COMPONENT_LIB} PROPERTY CXX_STANDARD 23)


# if(CONFIG_COMPILER_OPTIMIZATION_DEBUG)
#   message(STATUS "[x] Building with DEBUG")
#   target_compile_definitions(${COMPONENT_LIB} PUBLIC 
#     "-DDEBUGLOG_DEFAULT_LOG_LEVEL_NONE"
#     "-DCORE_DEBUG_LEVEL=2"
#     "-DLOG_LOCAL_LEVEL=ESP_LOG_VERBOSE"
#   )

#   target_compile_options(${COMPONENT_LIB} PRIVATE 
#     "-Wall" 
#     "-Wpedantic" 
#     "-Wextra" 
#   )
# else()
#   message(STATUS "[ ] Building without DEBUG")
#   target_compile_definitions(${COMPONENT_LIB} PUBLIC
#     "-DDEBUGLOG_DEFAULT_LOG_LEVEL_NONE"
#     "-DLOG_LOCAL_LEVEL=1"
#     "-DCORE_DEBUG_LEVEL=1"
#   )
# endif()

message(STATUS "main CMAKE_CXX_FLAGS_RELEASE: ${CMAKE_CXX_FLAGS_RELEASE}")
message(STATUS "main CMAKE_CXX_FLAGS: ${CMAKE_CXX_FLAGS}")
message(STATUS "main Compile options: ${COMPILE_OPTIONS}")
message(STATUS "main Compile definitions: ${COMPILE_DEFINITIONS}")
message(STATUS "main Link options: ${LINK_OPTIONS}")

