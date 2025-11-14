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

message(STATUS "[x] IO Manager Component Registered.")

# add_custom_command(
#     TARGET ${COMPONENT_LIB}
#     POST_BUILD
#     COMMAND ls cache/
#     WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
#     COMMENT ">>>>>>>>>>>>>>>>>>>>>>>>>>> Running custom script after build"
# )

# idf_component_register(SRC_DIRS "." INCLUDE_DIRS "include")
# target_include_directories(${COMPONENT_LIB} INTERFACE "include")

# Generate iom_config.h based on what's available
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/iom_config.h.in" 
"#pragma once
#define IOM_I2C_SDA_PIN @IOM_I2C_SDA_PIN@
#define IOM_I2C_SCL_PIN @IOM_I2C_SCL_PIN@
#define IOM_I2C_FREQUENCY @IOM_I2C_FREQUENCY@
")

# Use IMPL pins if available, otherwise defaults
if(DEFINED CONFIG_IMPL_I2C_SDA_PIN)
    set(IOM_I2C_SDA_PIN ${CONFIG_IMPL_I2C_SDA_PIN})
else()
    set(IOM_I2C_SDA_PIN 21)
endif()

if(DEFINED CONFIG_IMPL_I2C_SCL_PIN)
    set(IOM_I2C_SCL_PIN ${CONFIG_IMPL_I2C_SCL_PIN})
else()
    set(IOM_I2C_SCL_PIN 22)
endif()

set(IOM_I2C_FREQUENCY ${CONFIG_IOM_I2C_FREQUENCY})

configure_file(
    "${CMAKE_CURRENT_BINARY_DIR}/iom_config.h.in"
    "${CMAKE_CURRENT_BINARY_DIR}/iom_config.h"
    @ONLY
)

target_include_directories(${COMPONENT_LIB} PRIVATE "${CMAKE_CURRENT_BINARY_DIR}")