cmake_minimum_required(VERSION 3.5)

project(LightingIO VERSION 0.2.1 LANGUAGES C CXX)

# for generate ycm complete database
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CONFIG_DIR ${LightingIO_SOURCE_DIR}/cmake)
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CONFIG_DIR}/Modules)

option(LTIO_BUILD_EXAMPLES "Build UNITTEST" ON)
option(LTIO_BUILD_UNITTESTS "Build UNITTEST" OFF)
option(LTIO_BUILD_SHARED_LIBS "Build shared libraries" ON)

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
ADD_SUBDIRECTORY(unittests)

MESSAGE(STATUS "SOURCE DIR: " ${LightingIO_SOURCE_DIR})
MESSAGE(STATUS "BINARY DIR: " ${LightingIO_BINARY_DIR})
MESSAGE(STATUS "OUTPUT lib DIR: " ${CMAKE_LIBRARY_OUTPUT_DIRECTORY})
MESSAGE(STATUS "OUTPUT bin DIR: " ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
