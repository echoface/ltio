cmake_minimum_required(VERSION 3.14 FATAL_ERROR)

project(LightingIO VERSION 0.2.1 LANGUAGES C CXX)

# for generate ycm complete database
SET(CMAKE_CXX_STANDARD 14)
SET(CMAKE_EXPORT_COMPILE_COMMANDS ON)

SET(CMAKE_CONFIG_DIR ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CONFIG_DIR})
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CONFIG_DIR}/Modules)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING "Choose the build type" FORCE)

  # Include "None" as option to disable any additional (optimization) flags,
  # relying on just CMAKE_C_FLAGS and CMAKE_CXX_FLAGS (which are empty by
  # default). These strings are presented in cmake-gui.
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "None" "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif()
if(NOT CMAKE_C_FLAGS)
    set(CMAKE_C_FLAGS "-O2 -g")
    set(CMAKE_C_FLAGS_DEBUG "-O0 -g")
    set(CMAKE_C_FLAGS_RELEASE "-O2 -g")
endif()
if(NOT CMAKE_CXX_FLAGS)
    set(CMAKE_CXX_FLAGS "-O2 -g")
    set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g")
    set(CMAKE_CXX_FLAGS_RELEASE "-O2 -g")
endif()

# build
option(LTIO_BUILD_EXAMPLES "Build UNITTEST" ON)
option(LTIO_BUILD_UNITTESTS "Build UNITTEST" ON)
option(LTIO_BUILD_SHARED_LIBS "Build shared libraries" ON)

# features
option(LTIO_WITH_HTTP2 "enable http2 support" ON)
option(LTIO_WITH_OPENSSL "support ssl by openssl" ON)
option(LTIO_USE_SYS_NGHTTP2 "use system wide installed nghttp2 libraries" OFF)

option(LTIO_ENABLE_REUSER_PORT "enable reuse port" ON)

include(${CMAKE_CONFIG_DIR}/cpm.cmake)
include(${CMAKE_CONFIG_DIR}/common.cmake)
include(${CMAKE_CONFIG_DIR}/third_deps.cmake)

include(${CMAKE_CONFIG_DIR}/targets.cmake) # declare targets

message(STATUS "summary of ltio build options:

    Install prefix:   ${CMAKE_INSTALL_PREFIX}
    Target system:    ${CMAKE_SYSTEM_NAME}
    Compiler:
      Build type:     ${CMAKE_BUILD_TYPE}
      C compiler:     ${CMAKE_C_COMPILER}
      C++ compiler:   ${CMAKE_CXX_COMPILER}
      CFLAGS:         ${CMAKE_C_FLAGS_${_build_type}} ${CMAKE_C_FLAGS}
      CXXFLAGS:       ${CMAKE_CXX_FLAGS_${_build_type}} ${CMAKE_CXX_FLAGS}
      WARNCFLAGS:     ${WARNCFLAGS}
      CXX1XCXXFLAGS:  ${CXX1XCXXFLAGS}
    Features:
      ReusePorts:     ${LTIO_WITH_HTTP2}
      WitOpenSSL:     ${LTIO_WITH_OPENSSL}
      EnableHttp2:    ${LTIO_BUILD_SHARED_LIBS}
      BuildExamples:  ${LTIO_BUILD_EXAMPLES}
      BuildUnittest:  ${LTIO_BUILD_UNITTESTS}
      BuildSharedLib: ${LTIO_BUILD_SHARED_LIBS}
")
