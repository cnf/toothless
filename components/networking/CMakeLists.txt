file(GLOB_RECURSE SRC "*.cpp" "*.c")

set(INCL "include")
set(PRIVREQ
  FunLog
  esp_wifi
  esp_netif
  esp_http_client
  esp_https_ota
  esp_http_server
  app_update
  nvs_flash
)
set(REQ 
  pubsub-c
  configuration_manager
)

set(EMBED_FILES
  "${CMAKE_CURRENT_SOURCE_DIR}/favicon.ico"
)


idf_component_register(SRCS ${SRC} INCLUDE_DIRS ${INCL} REQUIRES ${REQ} PRIV_REQUIRES ${PRIVREQ} EMBED_FILES ${EMBED_FILES})

message(STATUS "[x] Network Manager Component Registered.")
