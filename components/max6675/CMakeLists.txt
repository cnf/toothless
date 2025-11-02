# set(SRC funlog_write.c)
file(GLOB_RECURSE SRC "*.cpp" "*.c")

# set(INCL "${CMAKE_SOURCE_DIR}/include") # "${INCLUDE_DIRECTORIES}")
set(INCL ".")
set(PRIVREQ 
  esp_driver_gpio
  FunLog
)
set(REQ 
)
idf_component_register(SRCS ${SRC} INCLUDE_DIRS ${INCL} REQUIRES ${REQ} PRIV_REQUIRES ${PRIVREQ})
message(STATUS "MAX6675")

# idf_component_register(SRC_DIRS "." INCLUDE_DIRS "include")
# target_include_directories(${COMPONENT_LIB} INTERFACE "include")
