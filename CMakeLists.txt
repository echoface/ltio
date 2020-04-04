cmake_minimum_required(VERSION 3.5)

project(LightingIO)

# for generate ycm complete database
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CONFIG_DIR ${LightingIO_SOURCE_DIR}/cmake)
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CONFIG_DIR}/Modules)
MESSAGE(STATUS "CMAKE_MODULE_PATH: ${CMAKE_MODULE_PATH}")

include(ExternalProject)
include(GNUInstallDirs)
include(${CMAKE_CONFIG_DIR}/common.cmake)
include(${CMAKE_CONFIG_DIR}/dependency.cmake)
include(${CMAKE_CONFIG_DIR}/include.cmake)
#include(${CMAKE_CONFIG_DIR}/googletest.cmake)

ADD_SUBDIRECTORY(thirdparty)
ADD_SUBDIRECTORY(base)
ADD_SUBDIRECTORY(net_io)
ADD_SUBDIRECTORY(components)
ADD_SUBDIRECTORY(integration)
ADD_SUBDIRECTORY(playground)

MESSAGE(STATUS "SOURCE DIR: " ${LightingIO_SOURCE_DIR})
MESSAGE(STATUS "BINARY DIR: " ${LightingIO_BINARY_DIR})
MESSAGE(STATUS "OUTPUT lib DIR: " ${CMAKE_LIBRARY_OUTPUT_DIRECTORY})
MESSAGE(STATUS "OUTPUT bin DIR: " ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
