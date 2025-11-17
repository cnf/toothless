file(GLOB_RECURSE SRC "src/*.cpp" "src/*.c")
set(INCL "include")
set(PRIVREQ 
  FunLog
  esp_lcd
  esp_lvgl_port
  esp_timer
  lvgl
  # esp_app_format
)
set(REQ 
)
# if (NOT DEFINED CONFIG_SPIRAM)
#   message(FATAL_ERROR "LilyGo T-HMI implementation requires SPIRAM to be enabled")
# endif()
if(CONFIG_IMPL_LILYGO_T_HMI)
  message(STATUS "[x] Impl: LilyGo T-HMI display implementation")
  set(CONFIG_IMPL_DISPLAY_HRES ${CONFIG_IMPL_LILYGO_T_HMI_HRES})
  set(CONFIG_IMPL_DISPLAY_VRES ${CONFIG_IMPL_LILYGO_T_HMI_VRES})
  idf_component_register(SRCS ${SRC} INCLUDE_DIRS ${INCL} REQUIRES ${REQ} PRIV_REQUIRES ${PRIVREQ})
else()
  idf_component_register(SRCS "" INCLUDE_DIRS "" REQUIRES "" PRIV_REQUIRES ${PRIVREQ})
endif()
# idf_component_register(SRC_DIRS "." INCLUDE_DIRS "include")
# target_include_directories(${COMPONENT_LIB} INTERFACE "include")
