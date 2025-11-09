file(GLOB_RECURSE SRC "*.cpp" "*.c")

# set(INCL "${CMAKE_SOURCE_DIR}/include") # "${INCLUDE_DIRECTORIES}")
set(INCL "include")
set(PRIVREQ
  FunLog

)
set(REQ 
  esp_driver_i2c
  # pubsub-c
)
idf_component_register(SRCS ${SRC} INCLUDE_DIRS ${INCL} REQUIRES ${REQ} PRIV_REQUIRES ${PRIVREQ})
message(STATUS "Hardware")

# idf_component_register(SRC_DIRS "." INCLUDE_DIRS "include")
# target_include_directories(${COMPONENT_LIB} INTERFACE "include")
