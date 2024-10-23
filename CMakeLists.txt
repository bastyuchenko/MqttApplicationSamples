cmake_minimum_required(VERSION 3.10)

project(mqtt_samples LANGUAGES C CXX)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED True)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

option(LOG_ALL_MOSQUITTO "Print all mosquitto logs" OFF)
option(ENABLE_UNIT_TESTS "Build unit tests" OFF)

if(LOG_ALL_MOSQUITTO)
  add_compile_definitions(LOG_ALL_MOSQUITTO)
endif()

include(FetchContent)

if(ENABLE_UNIT_TESTS)
  include(CTest)
endif()

# Mosquitto library setup
option(DOCUMENTATION OFF)
option(WITH_CJSON OFF)
FetchContent_Declare(
    MOSQUITTO
    GIT_REPOSITORY https://github.com/eclipse/mosquitto.git
    GIT_TAG        v2.0.15
    GIT_PROGRESS   TRUE
)

if(NOT MOSQUITTO_PATH)
  set(FETCHCONTENT_QUIET FALSE)
  FetchContent_MakeAvailable(MOSQUITTO)
  set(MOSQUITTO_PATH ${mosquitto_SOURCE_DIR})
endif()

message(INFO "MOSQUITTO_PATH set to ${MOSQUITTO_PATH}")

link_libraries(mosquitto)

set(MOSQUITTO_CLIENT_EXTENSIONS_DIR ${CMAKE_CURRENT_LIST_DIR}/mqttclients/c/mosquitto_client_extensions)
file(GLOB MOSQUITTO_CLIENT_EXTENSIONS ${MOSQUITTO_CLIENT_EXTENSIONS_DIR}/*.c)
include_directories(${MOSQUITTO_CLIENT_EXTENSIONS_DIR})

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

add_subdirectory(${PRESET_PATH})