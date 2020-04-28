cmake_minimum_required(VERSION 3.5)

project(LightingIO VERSION 0.2.1 LANGUAGES C CXX)

# for generate ycm complete database
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CONFIG_DIR ${LightingIO_SOURCE_DIR}/cmake)
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CONFIG_DIR}/Modules)

option(LTIO_BUILD_EXAMPLES "Build UNITTEST" ON)
option(LTIO_BUILD_UNITTESTS "Build UNITTEST" ON)
option(LTIO_BUILD_SHARED_LIBS "Build shared libraries" ON)
option(LTIO_USE_ACO_CORO_IMPL "enabel ltaco coroutine implement" OFF)
option(LTIO_DISABLE_ASYNCMYSQL "enabel ltasyncmysql integration" OFF)

#include(external/CMakeLists.txt)

include(ExternalProject)
include(GNUInstallDirs)
include(${CMAKE_CONFIG_DIR}/common.cmake)
include(${CMAKE_CONFIG_DIR}/dependency.cmake)

include(${CMAKE_CONFIG_DIR}/include.cmake)
#include(${CMAKE_CONFIG_DIR}/googletest.cmake)
include(${CMAKE_CONFIG_DIR}/targets.cmake)



