file(GLOB_RECURSE SRC "*.cpp" "*.c")

set(INCL "include")

set(PRIVREQ
  FunLog
  pubsub-c
)

set(REQ
  esp_driver_uart
)

idf_component_register(
  SRCS ${SRC}
  INCLUDE_DIRS ${INCL}
  REQUIRES ${REQ}
  PRIV_REQUIRES ${PRIVREQ}
)

message(STATUS "[x] Pubsub Bridge Component Registered")
