file(GLOB_RECURSE SRC "src/*.cpp" "src/*.c")
set(INCL "include")
set(PRIVREQ 
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
# if (NOT DEFINED CONFIG_SPIRAM)
#   message(FATAL_ERROR "LilyGo T-HMI implementation requires SPIRAM to be enabled")
# endif()
if(CONFIG_IMPL_M5DIAL)
  message(STATUS "[x] Impl: M5Stack Dial display implementation")
  set(CONFIG_IMPL_DISPLAY_HRES ${CONFIG_IMPL_M5DIAL_HRES})
  set(CONFIG_IMPL_DISPLAY_VRES ${CONFIG_IMPL_M5DIAL_VRES})
  idf_component_register(SRCS ${SRC} INCLUDE_DIRS ${INCL} REQUIRES ${REQ} PRIV_REQUIRES ${PRIVREQ})
else()
  idf_component_register(SRCS "" INCLUDE_DIRS "" REQUIRES "" PRIV_REQUIRES ${PRIVREQ})
endif()
# idf_component_register(SRC_DIRS "." INCLUDE_DIRS "include")
# target_include_directories(${COMPONENT_LIB} INTERFACE "include")


# # set(SRC funlog_write.c)
# file(GLOB_RECURSE SRC "src/*.cpp" "src/*.c")

# # set(INCL "${CMAKE_SOURCE_DIR}/include") # "${INCLUDE_DIRECTORIES}")
# if(CONFIG_IMPL_M5DIAL)
#   message(STATUS "[x] M5Stack Dial display implementation")
#   set(INCL "include")
#   set(SOURCES ${SRC})
# else()
#   set(INCL "")
#   set(SOURCES "")
# endif()
# # set(INCL "include")
# set(PRIVREQ 
#   FunLog
#   esp_lcd
#   esp_lvgl_port
#   esp_driver_spi
#   esp_timer
#   lvgl
#   # esp_app_format
# )
# set(REQ 
# )
# idf_component_register(SRCS ${SRC} INCLUDE_DIRS ${INCL} REQUIRES ${REQ} PRIV_REQUIRES ${PRIVREQ})

# # idf_component_register(SRC_DIRS "." INCLUDE_DIRS "include")
# # target_include_directories(${COMPONENT_LIB} INTERFACE "include")
