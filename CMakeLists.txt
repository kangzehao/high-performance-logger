cmake_minimum_required(VERSION 3.16)
project(logger)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
    message(STATUS "No build type selected, defaulting to ${CMAKE_BUILD_TYPE}")
endif()

#add_definitions(-DENABLE_LOG=1)

include(${CMAKE_CURRENT_LIST_DIR}/build/conandeps_legacy.cmake)

# include header
include_directories(${PROJECT_SOURCE_DIR}/)

if(NOT "$ENV{LOGGER_VERSION}" STREQUAL "")
  set(LOGGER_VERSION "$ENV{LOGGER_VERSION}")
else()
  set(LOGGER_VERSION "U.0.0.0")
  add_definitions(-DTEST_METHOD)
endif()

message("logger version: " ${LOGGER_VERSION})
add_definitions(-LOGGER_VERSION=${LOGGER_VERSION})

# c++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_VERBOSE_MAKEFILE ON CACHE BOOL "ON")

if(UNIX AND NOT ANDROID_NDK)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wno-unknown-pragmas")
endif()

add_subdirectory(${PROJECT_SOURCE_DIR}/src)

add_subdirectory(${PROJECT_SOURCE_DIR}/examples)

add_subdirectory(${PROJECT_SOURCE_DIR}/decode)

add_subdirectory(${PROJECT_SOURCE_DIR}/benchmark)

if("$ENV{LOGGER_VERSION}" STREQUAL "")
    enable_testing()
    add_subdirectory(${PROJECT_SOURCE_DIR}/tests)
endif()

string(TOUPPER "${CMAKE_BUILD_TYPE}" UPPERCASE_BUILD_TYPE)

if(UPPERCASE_BUILD_TYPE STREQUAL "DEBUG")
    message(STATUS "Build Type is Debug: Enabling ASan and -g")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address -g")
else()
    message(STATUS "Build Type is ${CMAKE_BUILD_TYPE}: ASan DISABLED for performance")
endif()
