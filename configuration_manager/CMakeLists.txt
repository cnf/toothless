# set(SRC funlog_write.c)
file(GLOB_RECURSE SRC "*.cpp" "*.c")

# set(INCL "${CMAKE_SOURCE_DIR}/include") # "${INCLUDE_DIRECTORIES}")
set(INCL "include")
set(PRIVREQ 
  esp_app_format
  pubsub-c
  FunLog
  nvs_flash
)
set(REQ 
 

)
idf_component_register(SRCS ${SRC} INCLUDE_DIRS ${INCL} REQUIRES ${REQ} PRIV_REQUIRES ${PRIVREQ})
message(STATUS "Configuration Manager")

# idf_component_register(SRC_DIRS "." INCLUDE_DIRS "include")
# target_include_directories(${COMPONENT_LIB} INTERFACE "include")
