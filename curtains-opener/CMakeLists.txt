cmake_minimum_required(VERSION 2.8.4)
set (CMAKE_CXX_FLAGS "--std=gnu++11 ${CMAKE_CXX_FLAGS}")

#target_compile_features(${PROJECT_NAME} PRIVATE cxx_range_for)
#
set(CMAKE_TOOLCHAIN_FILE ${CMAKE_SOURCE_DIR}/../arduino-cmake/cmake/ArduinoToolchain.cmake)

#include_directories(../Arduino-logging-library)
#link_directories(../arduino-base)
include_directories(../arduino-base)
set(HEADER_FILES ../arduino-base/Base.h ${HEADER_FILES})
set(PROJECT_NAME curtains)

project(${PROJECT_NAME})

set(${CMAKE_PROJECT_NAME}_SRCS Main.cpp)
set(${CMAKE_PROJECT_NAME}_BOARD uno)
set(${CMAKE_PROJECT_NAME}_PORT /dev/ttyACM0)

#generate_arduino_example(${CMAKE_PROJECT_NAME})
generate_arduino_firmware(${CMAKE_PROJECT_NAME})
#file(GLOB SOURCES *.cpp)