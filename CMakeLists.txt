cmake_minimum_required(VERSION 3.14 FATAL_ERROR)

project(LightingIO VERSION 0.2.1 LANGUAGES C CXX)

# for generate ycm complete database
SET(CMAKE_CXX_STANDARD 14)
SET(CMAKE_EXPORT_COMPILE_COMMANDS ON)

SET(CMAKE_CONFIG_DIR ${LightingIO_SOURCE_DIR}/cmake)
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CONFIG_DIR}/Modules)

# build
option(LTIO_BUILD_EXAMPLES "Build UNITTEST" ON)
option(LTIO_BUILD_UNITTESTS "Build UNITTEST" ON)
option(LTIO_BUILD_SHARED_LIBS "Build shared libraries" ON)

# features
option(LTIO_WITH_HTTP2 "enable http2 support" ON)
option(LTIO_WITH_OPENSSL "support ssl by openssl" ON)
option(LTIO_USE_SYS_NGHTTP2 "use system wide installed nghttp2 libraries" OFF)

# switchs
option(LTIO_ENABLE_REUSER_PORT "enable reuse port" ON)

include(ExternalProject)
include(GNUInstallDirs)

include(${CMAKE_CONFIG_DIR}/CPM.cmake)
include(${CMAKE_CONFIG_DIR}/common.cmake)
include(${CMAKE_CONFIG_DIR}/dependency.cmake)
include(${CMAKE_CONFIG_DIR}/subdirectory.cmake)
#include(${CMAKE_CONFIG_DIR}/googletest.cmake)
include(${CMAKE_CONFIG_DIR}/targets.cmake)
