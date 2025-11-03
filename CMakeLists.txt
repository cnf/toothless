cmake_minimum_required(VERSION 3.20.0)
set(CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_compile_options(-fdiagnostics-color=always)
# add_compile_definitions(DEBUGLOG_DEFAULT_LOG_LEVEL_DEBUG)
# add_compile_definitions(CNF_ROOT)
# list(APPEND EXTRA_COMPONENT_DIRS ${CMAKE_SOURCE_DIR}/external)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
include_directories(${CMAKE_CURRENT_LIST_DIR}/include)
if (NOT CONFIG_COMPILER_OPTIMIZATION_DEBUG)
  idf_build_set_property(MINIMAL_BUILD ON)
endif()

project(Toothless LANGUAGES CXX)

# idf_build_set_property(
#   COMPILE_OPTIONS
#   "-w"
#   APPEND
# )

# Add component-specific suppressions
# idf_component_set_property(lvgl__lvgl COMPILE_OPTIONS "-w" APPEND)
# idf_component_set_property(QDispatch COMPILE_OPTIONS "-w" APPEND) 
idf_component_set_property(pubsub-c COMPILE_OPTIONS "-w" APPEND)
idf_component_set_property(atanisoft__esp_lcd_touch_xpt2046 COMPILE_OPTIONS "-w" APPEND)
idf_component_set_property(espressif__esp32_display_panel COMPILE_OPTIONS "-w" APPEND)


## Link time optimization
idf_build_set_property(
  COMPILE_OPTIONS
  "-ffunction-sections" 
  "-fdata-sections"
  "-flto"
  APPEND
)
idf_build_set_property(
  LINK_OPTIONS
  "-ffunction-sections" 
  "-fdata-sections"
  "--gc-sections"
  "-flto"
  APPEND
)

# idf_build_set_property(COMPILE_OPTIONS "-DDEBUGLOG_DEFAULT_LOG_LEVEL_DEBUG" "-DCORE_DEBUG_LEVEL=2" "-DLOG_LOCAL_LEVEL=5" "-DESP_LOG_LEVEL_LOCAL=5" "-DESP_CORE_DEBUG_LEVEL=2" APPEND)

# idf_build_set_property(COMPILE_OPTIONS "-Os" APPEND)

# idf_build_set_property(COMPILE_OPTIONS "-Os" "-Wno-error" "-Wall" "-DDEBUGLOG_DEFAULT_LOG_LEVEL_DEBUG" APPEND)
# -O0 -g3 -ggdb3

# set(CMAKE_CXX_FLAGS_INIT "-DDEBUGLOG_DEFAULT_LOG_LEVEL_DEBUG")
# set(CMAKE_CXX_FLAGS_DEBUG_INIT "-Wall -Wno-error -O0 -g3 -ggdb3 -DDEBUGLOG_DEFAULT_LOG_LEVEL_DEBUG")
# set(CMAKE_CXX_FLAGS_RELEASE_INIT "-Wall -Wno-error -DDEBUGLOG_DEFAULT_LOG_LEVEL_ERROR -DDEBUGLOG_DISABLE_LOG")

# message(VERBOSE "CMAKE_CXX_FLAGS_RELEASE: ${CMAKE_CXX_FLAGS_RELEASE}")
# message(VERBOSE "Compile options: ${COMPILE_OPTIONS}")
# message(VERBOSE "Link options: ${LINK_OPTIONS}")
