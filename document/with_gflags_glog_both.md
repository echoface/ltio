## 项目中同时使用glog 和 gflags

这两个项目作为依赖同时使用让我恼火了很久。cmake这个老古董在
C/c++上真的是一言难尽啊，前年老大难各种奇怪问题。你没遇到？
你功夫好，姿势多。

- https://github.com/google/glog/issues/251
- https://github.com/google/glog/issues/560
- https://github.com/google/glog/issues/198
- https://github.com/google/glog/issues/381
- https://github.com/gflags/gflags/issues/279
....

目前参考Fuchsia子项目的一种使用方式。colbat项目的使用方式如下
```
# Build gflags as an external project.
set(GFLAGS_INSTALL_DIR ${CMAKE_BINARY_DIR}/third_party/gflags)
set(GFLAGS_INCLUDE_DIR ${GFLAGS_INSTALL_DIR}/include)
set(GFLAGS_LIB_DIR ${GFLAGS_INSTALL_DIR}/lib)
ExternalProject_Add(gflags_external_project
                    SOURCE_DIR  ${CMAKE_SOURCE_DIR}/third_party/gflags
                    PREFIX      ${GFLAGS_INSTALL_DIR}
                    INSTALL_DIR ${GFLAGS_INSTALL_DIR}
                    CMAKE_ARGS  -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}
                                -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
                                -DCMAKE_CXX_FLAGS=${EXTERNAL_PROJECT_CMAKE_CXX_FLAGS}
                                -DCMAKE_INSTALL_PREFIX:PATH=${GFLAGS_INSTALL_DIR}
                   )
include_directories(BEFORE SYSTEM ${GFLAGS_INCLUDE_DIR})
link_directories(${GFLAGS_LIB_DIR})

# Build glog as an external project.
set(GLOG_INSTALL_DIR ${CMAKE_BINARY_DIR}/third_party/glog)
set(GLOG_INCLUDE_DIR ${GLOG_INSTALL_DIR}/include)
set(GLOG_LIB_DIR ${GLOG_INSTALL_DIR}/lib)
ExternalProject_Add(glog_external_project
                    SOURCE_DIR  ${CMAKE_SOURCE_DIR}/third_party/glog
                    PREFIX      ${GLOG_INSTALL_DIR}
                    INSTALL_DIR ${GLOG_INSTALL_DIR}
                    CMAKE_ARGS  -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}
                                -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
                                -DCMAKE_CXX_FLAGS=${EXTERNAL_PROJECT_CMAKE_CXX_FLAGS}
                                -Dcobalt_gflags_DIR=${GFLAGS_INSTALL_DIR}
                                -DCMAKE_INSTALL_PREFIX:PATH=${GLOG_INSTALL_DIR}
                    DEPENDS     gflags_external_project
                   )
include_directories(BEFORE SYSTEM ${GLOG_INCLUDE_DIR})
link_directories(${GLOG_LIB_DIR})

add_dependencies(my_program
    gflags_external_project
    glog_external_project
    )
target_link_libraries(my_program pthread gflags glog)
```


```Cmake
# Copyright 2016 The Fuchsia Authors
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#    http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
cmake_minimum_required (VERSION 3.0.0)
# The only way we found to override the compiler is setting these variables
# prior to the project definition.  Otherwise enable_language() sets these
# variables and gcc will be picked up instead.  Also, new versions of cmake will
# complain if one compiler is first set (say to gcc via enable_language), and
# then we later switch it to (say) clang.
set(CMAKE_C_COMPILER "clang")
set(CMAKE_CXX_COMPILER "clang++")
project(cobalt)
include(ExternalProject)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -Wall -Wsign-compare -Wignored-qualifiers -std=c++14 -stdlib=libc++ -pthread -fcolor-diagnostics")
enable_language(C)
enable_language(CXX)
# Tell the file logging.h that we want to use Google's GLog library for
# logging.
add_definitions(-DHAVE_GLOG=1)
set(DIR_CONFIG_PARSER_TESTS "${CMAKE_BINARY_DIR}/config_parser_tests")
set(DIR_GTESTS "${CMAKE_BINARY_DIR}/gtests")
set(DIR_SYSROOT "${CMAKE_SOURCE_DIR}/sysroot")
set(DIR_END_TO_END_TESTS "${CMAKE_BINARY_DIR}/e2e_tests")
set(DIR_GTESTS_BT_EMULATOR "${CMAKE_BINARY_DIR}/gtests_btemulator")
set(DIR_GTESTS_CLOUD_BT "${CMAKE_BINARY_DIR}/gtests_cloud_bt")
set(DIR_PERF_TESTS "${CMAKE_BINARY_DIR}/perf_tests")
set(DIR_MANUAL_TESTS "${CMAKE_BINARY_DIR}/manual_tests")
# Go related defines
set(GO_PATH env GOPATH="${CMAKE_SOURCE_DIR}/third_party/go:${CMAKE_BINARY_DIR}/go-proto-gen")
set(GO_PATH env "${GO_PATH}:${CMAKE_SOURCE_DIR}/shuffler")
set(GO_PATH env "${GO_PATH}:${CMAKE_SOURCE_DIR}/config/config_parser")
set(GO_PATH env "${GO_PATH}:${CMAKE_SOURCE_DIR}/tools/go")
set(GO_BIN ${GO_PATH} go)
set(GO_TESTS "${CMAKE_BINARY_DIR}/go_tests")
set(GO_PROTO_GEN_SRC_DIR "${CMAKE_BINARY_DIR}/go-proto-gen/src")
file(MAKE_DIRECTORY ${GO_PROTO_GEN_SRC_DIR})
file(MAKE_DIRECTORY ${GO_TESTS})
link_directories(${DIR_SYSROOT}/lib)
include_directories(${CMAKE_SOURCE_DIR})
include_directories(${CMAKE_BINARY_DIR})
include_directories(BEFORE SYSTEM ${DIR_SYSROOT}/include)
# Build external projects
set(EXTERNAL_PROJECT_CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unused-local-typedef -Wno-sign-compare -Wno-deprecated-declarations -Wno-unused-private-field -Wno-ignored-qualifiers -Wno-deprecated-register -Wno-unused-function -Wno-unused-local-typedef -Wno-missing-field-initializers -Wno-enum-compare-switch -Wno-macro-redefined -Wno-implicit-function-declaration -Wno-unused-variable")
# Build googletest as an external project.
set(GTEST_INSTALL_DIR ${CMAKE_BINARY_DIR}/third_party/googletest/googletest)
set(GTEST_INCLUDE_DIR ${GTEST_INSTALL_DIR}/include)
set(GTEST_LIB_DIR ${GTEST_INSTALL_DIR}/lib)
ExternalProject_Add(gtest_external_project
                    SOURCE_DIR  ${CMAKE_SOURCE_DIR}/third_party/googletest/googletest
                    PREFIX      ${GTEST_INSTALL_DIR}
                    INSTALL_DIR ${GTEST_INSTALL_DIR}
                    CMAKE_ARGS  -DCMAKE_INSTALL_PREFIX:PATH=${GTEST_INSTALL_DIR}
                                -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}
                                -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
                                -DCMAKE_CXX_FLAGS=${EXTERNAL_PROJECT_CMAKE_CXX_FLAGS}
                   )
include_directories(BEFORE SYSTEM ${GTEST_INCLUDE_DIR})
link_directories(${GTEST_LIB_DIR})

# Build gflags as an external project.
set(GFLAGS_INSTALL_DIR ${CMAKE_BINARY_DIR}/third_party/gflags)
set(GFLAGS_INCLUDE_DIR ${GFLAGS_INSTALL_DIR}/include)
set(GFLAGS_LIB_DIR ${GFLAGS_INSTALL_DIR}/lib)
ExternalProject_Add(gflags_external_project
                    SOURCE_DIR  ${CMAKE_SOURCE_DIR}/third_party/gflags
                    PREFIX      ${GFLAGS_INSTALL_DIR}
                    INSTALL_DIR ${GFLAGS_INSTALL_DIR}
                    CMAKE_ARGS  -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}
                                -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
                                -DCMAKE_CXX_FLAGS=${EXTERNAL_PROJECT_CMAKE_CXX_FLAGS}
                                -DCMAKE_INSTALL_PREFIX:PATH=${GFLAGS_INSTALL_DIR}
                   )
include_directories(BEFORE SYSTEM ${GFLAGS_INCLUDE_DIR})
link_directories(${GFLAGS_LIB_DIR})

# Build glog as an external project.
set(GLOG_INSTALL_DIR ${CMAKE_BINARY_DIR}/third_party/glog)
set(GLOG_INCLUDE_DIR ${GLOG_INSTALL_DIR}/include)
set(GLOG_LIB_DIR ${GLOG_INSTALL_DIR}/lib)
ExternalProject_Add(glog_external_project
                    SOURCE_DIR  ${CMAKE_SOURCE_DIR}/third_party/glog
                    PREFIX      ${GLOG_INSTALL_DIR}
                    INSTALL_DIR ${GLOG_INSTALL_DIR}
                    CMAKE_ARGS  -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}
                                -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
                                -DCMAKE_CXX_FLAGS=${EXTERNAL_PROJECT_CMAKE_CXX_FLAGS}
                                -Dcobalt_gflags_DIR=${GFLAGS_INSTALL_DIR}
                                -DCMAKE_INSTALL_PREFIX:PATH=${GLOG_INSTALL_DIR}
                    DEPENDS     gflags_external_project
                   )
include_directories(BEFORE SYSTEM ${GLOG_INCLUDE_DIR})
link_directories(${GLOG_LIB_DIR})
# Build boringssl as an external project.
set(BORINGSSL_INSTALL_DIR ${CMAKE_BINARY_DIR}/third_party/boringssl)
set(BORINGSSL_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/third_party/boringssl/include)
set(CRYPTO_LIB_DIR ${BORINGSSL_INSTALL_DIR}/src/boringssl_external_project-build/crypto)
set(SSL_LIB_DIR ${BORINGSSL_INSTALL_DIR}/src/boringssl_external_project-build/ssl)
ExternalProject_Add(boringssl_external_project
                    SOURCE_DIR  ${CMAKE_SOURCE_DIR}/third_party/boringssl
                    PREFIX      ${BORINGSSL_INSTALL_DIR}
                    INSTALL_DIR ${BORINGSSL_INSTALL_DIR}
                    CMAKE_ARGS  -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}
                                -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
                                -DCMAKE_CXX_FLAGS=${EXTERNAL_PROJECT_CMAKE_CXX_FLAGS}
                                -DCMAKE_INSTALL_PREFIX:PATH=${BORINGSSL_INSTALL_DIR}
                    DEPENDS     gflags_external_project
                    DEPENDS     glog_external_project
                    DEPENDS     gtest_external_project
                    INSTALL_COMMAND ""
                   )
include_directories(BEFORE SYSTEM ${BORINGSSL_INCLUDE_DIR})
link_directories(${CRYPTO_LIB_DIR})
link_directories(${SSL_LIB_DIR})
# Build protobuf as an external project.
set(PROTOBUF_INSTALL_DIR ${CMAKE_BINARY_DIR}/third_party/protobuf)
set(PROTOBUF_INCLUDE_DIR ${PROTOBUF_INSTALL_DIR}/include)
set(PROTOBUF_LIB_DIR ${PROTOBUF_INSTALL_DIR}/lib)
set(PROTOC ${PROTOBUF_INSTALL_DIR}/bin/protoc)
ExternalProject_Add(protobuf_external_project
                    SOURCE_DIR  ${CMAKE_SOURCE_DIR}/third_party/protobuf
                    PREFIX      ${PROTOBUF_INSTALL_DIR}
                    INSTALL_DIR ${PROTOBUF_INSTALL_DIR}
                    CMAKE_ARGS  -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}
                                -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
                                -DCMAKE_CXX_FLAGS=${EXTERNAL_PROJECT_CMAKE_CXX_FLAGS}
                                -DCMAKE_INSTALL_PREFIX:PATH=${PROTOBUF_INSTALL_DIR}
                   )
include_directories(BEFORE SYSTEM ${PROTOBUF_INCLUDE_DIR})
link_directories(${PROTOBUF_LIB_DIR})
# In the CMake build for protobuf we name the full protobuf library protobuf_full_cobalt.
set(PROTOBUF_FULL_LIB_NAME protobuf_full_cobalt)
# Build c-ares as an external project.
set(CARES_INSTALL_DIR ${CMAKE_BINARY_DIR}/third_party/cares)
set(CARES_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/third_party/cares)
set(CARES_LIB_DIR ${CARES_INSTALL_DIR}/src/CARES_external_project-build)
ExternalProject_Add(CARES_external_project
                    SOURCE_DIR  ${CMAKE_SOURCE_DIR}/third_party/cares
                    PREFIX      ${CARES_INSTALL_DIR}
                    CMAKE_ARGS  -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}
                                -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
                                -DCMAKE_CXX_FLAGS=${EXTERNAL_PROJECT_CMAKE_CXX_FLAGS}
                                -DCMAKE_INSTALL_PREFIX:PATH=${CARES_INSTALL_DIR}
                    INSTALL_COMMAND ""
                   )
include_directories(BEFORE SYSTEM ${CARES_INCLUDE_DIR})
link_directories(${CARES_LIB_DIR})
# Build zlib as an external project.
set(ZLIB_INSTALL_DIR ${CMAKE_BINARY_DIR}/third_party/zlib)
set(ZLIB_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/third_party/zlib)
set(ZLIB_LIB_DIR ${ZLIB_INSTALL_DIR}/src/ZLIB_external_project-build)
ExternalProject_Add(ZLIB_external_project
                    SOURCE_DIR  ${CMAKE_SOURCE_DIR}/third_party/zlib
                    PREFIX      ${ZLIB_INSTALL_DIR}
                    CMAKE_ARGS  -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}
                                -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
                                -DCMAKE_CXX_FLAGS=${EXTERNAL_PROJECT_CMAKE_CXX_FLAGS}
                                -DCMAKE_INSTALL_PREFIX:PATH=${ZLIB_INSTALL_DIR}
                    INSTALL_COMMAND ""
                   )
include_directories(BEFORE SYSTEM ${ZLIB_INCLUDE_DIR})
link_directories(${ZLIB_LIB_DIR})
# Build grpc as an external project.
set(GRPC_INSTALL_DIR ${CMAKE_BINARY_DIR}/third_party/grpc)
set(GRPC_INCLUDE_DIR ${GRPC_INSTALL_DIR}/include)
set(GRPC_LIB_DIR ${GRPC_INSTALL_DIR}/lib)
set(GRPC_CPP_PLUGIN ${GRPC_INSTALL_DIR}/bin/grpc_cpp_plugin)
ExternalProject_Add(grpc_external_project
                    SOURCE_DIR  ${CMAKE_SOURCE_DIR}/third_party/grpc
                    PREFIX      ${GRPC_INSTALL_DIR}
                    INSTALL_DIR ${GRPC_INSTALL_DIR}
                    CMAKE_ARGS  -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}
                                -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
                                -DCMAKE_CXX_FLAGS=${EXTERNAL_PROJECT_CMAKE_CXX_FLAGS}
                                -DCMAKE_INSTALL_PREFIX:PATH=${GRPC_INSTALL_DIR}
                                -DgRPC_ZLIB_PROVIDER=explicit
                                -DZLIB_ROOT_DIR=${CMAKE_CURRENT_SOURCE_DIR}/third_party/zlib
                                -D_gRPC_ZLIB_LIBRARIES=z
                                -DgRPC_ZLIB_LIB_DIR=${ZLIB_LIB_DIR}
                                -DgRPC_CARES_PROVIDER=explicit
                                -DCARES_ROOT_DIR=${CMAKE_CURRENT_SOURCE_DIR}/third_party/cares
                                -DgRPC_CARES_LIB_DIR=${CARES_LIB_DIR}
                                -DBENCHMARK_ROOT_DIR=${CMAKE_CURRENT_SOURCE_DIR}/third_party/benchmark
                                -DgRPC_PROTOBUF_PROVIDER=explicit
                                -DPROTOBUF_ROOT_DIR=${CMAKE_CURRENT_SOURCE_DIR}/third_party/protobuf
                                -DgRPC_PROTOBUF_PROTOC=${PROTOC}
                                -DgRPC_PROTOBUF_PROTOC_LIB=protoc_lib
                                -DgRPC_PROTOBUF_LIB=protobuf_lite
                                -DgRPC_PROTOBUF_FULL_LIB=${PROTOBUF_FULL_LIB_NAME}
                                -DgRPC_PROTOBUF_LIB_DIR=${PROTOBUF_LIB_DIR}
                                -DgRPC_USE_PROTO_LITE=1
                                -DgRPC_SSL_PROVIDER=explicit
                                -DBORINGSSL_ROOT_DIR=${CMAKE_CURRENT_SOURCE_DIR}/third_party/boringssl
                                -DgRPC_CRYPTO_LIBRARY=crypto
                                -DgRPC_SSL_LIBRARY=ssl
                                -DgRPC_CRYPTO_LIB_DIR=${CRYPTO_LIB_DIR}
                                -DgRPC_SSL_LIB_DIR=${SSL_LIB_DIR}
                                -DgRPC_GFLAGS_PROVIDER=explicit
                                -DgRPC_GFLAGS_LIB_DIR=${GFLAGS_LIB_DIR}
                    DEPENDS     protobuf_external_project
                    DEPENDS     boringssl_external_project
                   )
include_directories(BEFORE SYSTEM ${GRPC_INCLUDE_DIR})
link_directories(${GRPC_LIB_DIR})
link_directories(${GRPC_INSTALL_DIR}/src/grpc_external_project-build/third_party/cares)
# Build googleapis as an external project.
set(GOOGLEAPIS_INSTALL_DIR ${CMAKE_BINARY_DIR}/third_party/googleapis)
set(GOOGLEAPIS_INCLUDE_DIR ${GOOGLEAPIS_INSTALL_DIR}/include)
set(GOOGLEAPIS_LIB_DIR ${GOOGLEAPIS_INSTALL_DIR}/lib)
ExternalProject_Add(googleapis_external_project
                    SOURCE_DIR  ${CMAKE_SOURCE_DIR}/third_party/googleapis
                    PREFIX      ${GOOGLEAPIS_INSTALL_DIR}
                    INSTALL_DIR ${GOOGLEAPIS_INSTALL_DIR}
                    CMAKE_ARGS  -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}
                                -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
                                -DCMAKE_CXX_FLAGS=${EXTERNAL_PROJECT_CMAKE_CXX_FLAGS}
                                -DCMAKE_INSTALL_PREFIX:PATH=${GOOGLEAPIS_INSTALL_DIR}
                                -DPROTOC=${PROTOC}
                                -DGO_PROTO_GEN_SRC_DIR=${GO_PROTO_GEN_SRC_DIR}
                                -DPROTOBUF_INCLUDE_DIR=${PROTOBUF_INCLUDE_DIR}
                                -DGRPC_INCLUDE_DIR=${GRPC_INCLUDE_DIR}
                                -DPROTOBUF_LIB_DIR=${PROTOBUF_LIB_DIR}
                                -DGRPC_CPP_PLUGIN=${GRPC_CPP_PLUGIN}
                    DEPENDS     protobuf_external_project
                    DEPENDS     grpc_external_project
                   )
include_directories(BEFORE SYSTEM ${GOOGLEAPIS_INCLUDE_DIR})
link_directories(${GOOGLEAPIS_LIB_DIR})
# Build jsoncpp as an external project.
set(JSONCPP_INSTALL_DIR ${CMAKE_BINARY_DIR}/third_party/jsoncpp)
set(JSONCPP_INCLUDE_DIR ${JSONCPP_INSTALL_DIR}/include)
set(JSONCPP_LIB_DIR ${JSONCPP_INSTALL_DIR}/lib)
ExternalProject_Add(jsoncpp_external_project
                    SOURCE_DIR  ${CMAKE_SOURCE_DIR}/third_party/jsoncpp
                    PREFIX      ${JSONCPP_INSTALL_DIR}
                    INSTALL_DIR ${JSONCPP_INSTALL_DIR}
                    CMAKE_ARGS  -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}
                                -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
                                -DCMAKE_CXX_FLAGS=${EXTERNAL_PROJECT_CMAKE_CXX_FLAGS}
                                -DCMAKE_INSTALL_PREFIX:PATH=${JSONCPP_INSTALL_DIR}
                   )
include_directories(BEFORE SYSTEM ${JSONCPP_INCLUDE_DIR})
link_directories(${JSONCPP_LIB_DIR})
# Build curl as an external project.
set(CURL_INSTALL_DIR ${CMAKE_BINARY_DIR}/third_party/curl)
ExternalProject_Add(curl_external_project
                    SOURCE_DIR  ${CMAKE_SOURCE_DIR}/third_party/curl
                    PREFIX      ${CURL_INSTALL_DIR}
                    INSTALL_DIR ${CURL_INSTALL_DIR}
                    CMAKE_ARGS  -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}
                                -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
                                -DCMAKE_CXX_FLAGS=${EXTERNAL_PROJECT_CMAKE_CXX_FLAGS}
                                -DCMAKE_INSTALL_PREFIX:PATH=${CURL_INSTALL_DIR}
                                -DOPENSSL_INCLUDE_DIR=${BORINGSSL_INCLUDE_DIR}
                                -DZLIB_INCLUDE_DIR=${ZLIB_INCLUDE_DIR}
                    DEPENDS     boringssl_external_project
                    INSTALL_COMMAND ""
                   )
include_directories(BEFORE SYSTEM ${CMAKE_SOURCE_DIR}/third_party/curl/include)
include_directories(BEFORE SYSTEM ${CURL_INSTALL_DIR}/src/curl_external_project-build)
link_directories(${CURL_INSTALL_DIR}/src/curl_external_project-build)
# Build google_api_cpp_client as an external project.
set(GOOGLE_API_CPP_CLIENT_INSTALL_DIR ${CMAKE_BINARY_DIR}/third_party/google-api-cpp-client)
set(GOOGLE_API_CPP_CLIENT_INCLUDE_DIR ${GOOGLE_API_CPP_CLIENT_INSTALL_DIR}/src/google_api_cpp_client_external_project-build/include)
set(GOOGLE_API_CPP_CLIENT_LIB_DIR ${GOOGLE_API_CPP_CLIENT_INSTALL_DIR}/src/google_api_cpp_client_external_project-build/lib)
ExternalProject_Add(google_api_cpp_client_external_project
                    SOURCE_DIR  ${CMAKE_SOURCE_DIR}/third_party/google-api-cpp-client
                    PREFIX      ${GOOGLE_API_CPP_CLIENT_INSTALL_DIR}
                    INSTALL_DIR ${GOOGLE_API_CPP_CLIENT_INSTALL_DIR}
                    CMAKE_ARGS  -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}
                                -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
                                -DCMAKE_CXX_FLAGS=${EXTERNAL_PROJECT_CMAKE_CXX_FLAGS}
                                -DCMAKE_INSTALL_PREFIX:PATH=${GOOGLE_API_CPP_CLIENT_INSTALL_DIR}
                                -DCURL_INCLUDE_DIRS=${CMAKE_SOURCE_DIR}/third_party/curl/include
                                -DMORE_CURL_INCLUDE_DIRS=${CURL_INSTALL_DIR}/src/curl_external_project-build
                                -DGFLAGS_INCLUDE_DIRS=${GFLAGS_INCLUDE_DIR}
                                -DGLOG_INCLUDE_DIRS=${GLOG_INCLUDE_DIR}
                                -DGTEST_INCLUDE_DIRS=${GTEST_INCLUDE_DIR}
                                -DJSONCPP_INCLUDE_DIRS=${JSONCPP_INCLUDE_DIR}
                                -DOTHER_INCLUDE_DIRS=${BORINGSSL_INCLUDE_DIR}
                                -DHAVE_OPENSSL=1
                                -Dgoogleapis_build_samples=OFF
                                -Dgoogleapis_build_service_apis=ON
                    DEPENDS     gflags_external_project
                    DEPENDS     glog_external_project
                    DEPENDS     gtest_external_project
                    DEPENDS     jsoncpp_external_project
                    DEPENDS     curl_external_project
                   )
include_directories(${GOOGLE_API_CPP_CLIENT_INCLUDE_DIR})
include_directories(${CMAKE_SOURCE_DIR}/third_party/google-api-cpp-client/src)
include_directories(${CMAKE_SOURCE_DIR}/third_party/google-api-cpp-client/service_apis/storage)
link_directories(${GOOGLE_API_CPP_CLIENT_LIB_DIR})
# A target to combine all of the external projects.
add_custom_target(build_external_projects
                  DEPENDS gtest_external_project
                  DEPENDS gflags_external_project
                  DEPENDS glog_external_project
                  DEPENDS boringssl_external_project
                  DEPENDS protobuf_external_project
                  DEPENDS grpc_external_project
                  DEPENDS googleapis_external_project
                  DEPENDS google_api_cpp_client_external_project)
# Applying this macro to a target does two things:
# (i)  It causes build_external_projects to happen first before the target
#      is built.
# (ii) It adds link-time dependencies to the static libraries for all of
#      the third-party projects. You apply it as follows:
# add_base_dependencies(foo)
macro(add_base_dependencies target_name)
  add_dependencies(${target_name} build_external_projects)
  target_link_libraries(${target_name}
    # These are listed in dependency order. If a depends on b then
    # a should occur before b.
    grpc++
    grpc
    gpr
    ${PROTOBUF_FULL_LIB_NAME}
    glog
    gflags
    cares
    z
    ssl
    crypto
  )
endmacro()
# This macro should be applied to most library and exe targets in the cobalt
# build. Note that we separate out cobalt_proto_lib and config_proto_lib
# just so that we can apply the macro add_base_dependencies() when we
# build those two libraries. Every other library besides those two should
# depend on those two.
macro(add_cobalt_dependencies target_name)
  target_link_libraries(${target_name}
    cobalt_proto_lib
    config_proto_lib
  )
  add_base_dependencies(${target_name})
endmacro()
# Runs the protoc compiler on a set of .proto files to generate c++ files.
# Compiles the c++ files into a static library.
#
# Args:
# LIB_NAME: The name of the CMake target for the generated static library
# HDRS_OUT: A variable in which to write the list of names of the generated
#           header files
# USE_GRPC: A bool indicating whether or not use the gRPC plugin.
# <remaining args>: List of simple names of .proto files to include from the
#                   current source directory. The names should not include
#                   ".proto"
# example usage:
#
# cobalt_make_protobuf_cpp_lib(report_master_proto_lib
#                              REPORT_PROTO_HDRS
#                              true
#                              report_master report_internal)
#
# This will compile the files report_master.proto and report_internal.proto in
# the current source directory and generate a static library with a
# target name of report_master_proto_lib containing the ReportMaster gRPC
# service as well as the compiled protos from report_internal. The variable
# REPORT_PROTO_HDRS will contain the list of strings:
# { <some-path>/report_internal.grpc.pb.h
#   <some-path>/report_internal.pb.h
#   <some-path>/report_master.grpc.pb.h
#   <some-path>/report_master.pb.h}
# (Note that report_internal.proto does not contain a gRPC service definition
#  so that report_internal.grpc.pb.h is essentially empty.)
macro(cobalt_make_protobuf_cpp_lib LIB_NAME HDRS_OUT USE_GRPC)
  set(_protofiles)
  set(_generated_srcs)
  set(_generated_hdrs)
  foreach(name ${ARGN})
      list(APPEND _protofiles "${CMAKE_CURRENT_SOURCE_DIR}/${name}.proto")
      list(APPEND _generated_srcs "${CMAKE_CURRENT_BINARY_DIR}/${name}.pb.cc")
      list(APPEND _generated_hdrs "${CMAKE_CURRENT_BINARY_DIR}/${name}.pb.h")
      set_source_files_properties("${CMAKE_CURRENT_BINARY_DIR}/${name}.pb.cc" PROPERTIES GENERATED TRUE)
      set_source_files_properties("${CMAKE_CURRENT_BINARY_DIR}/${name}.pb.h" PROPERTIES GENERATED TRUE)
      if(${USE_GRPC})
        list(APPEND _generated_srcs "${CMAKE_CURRENT_BINARY_DIR}/${name}.grpc.pb.cc")
        list(APPEND _generated_hdrs "${CMAKE_CURRENT_BINARY_DIR}/${name}.grpc.pb.h")
        set_source_files_properties("${CMAKE_CURRENT_BINARY_DIR}/${name}.grpc.pb.cc" PROPERTIES GENERATED TRUE)
        set_source_files_properties("${CMAKE_CURRENT_BINARY_DIR}/${name}.grpc.pb.h" PROPERTIES GENERATED TRUE)
      endif()
  endforeach()
  set(_grpc_spec)
  if (${USE_GRPC})
    set(_grpc_spec
      --grpc_out=${CMAKE_BINARY_DIR}
      --plugin=protoc-gen-grpc=${GRPC_CPP_PLUGIN}
    )
  endif()
  add_custom_command(OUTPUT ${_generated_srcs} ${_generated_hdrs}
    COMMAND ${PROTOC} ${_protofiles}
            -I ${CMAKE_SOURCE_DIR}
            -I ${PROTOBUF_INCLUDE_DIR}
            --cpp_out=${CMAKE_BINARY_DIR}
            ${_grpc_spec}
    DEPENDS ${_protofiles}
    DEPENDS build_external_projects
  )
  add_library(${LIB_NAME}
    ${_generated_srcs}
  )
  if(${USE_GRPC})
    # This is a bit of a hack. We want to add dependencies on
    # cobalt_proto_lib and config_proto_lib as long as those aren't the
    # library we are currently building. We know those two are not gRPC
    # libraries.
    add_cobalt_dependencies(${LIB_NAME})
  else()
    add_base_dependencies(${LIB_NAME})
  endif()
  set(${HDRS_OUT} ${_generated_hdrs})
endmacro()
# Runs the protoc compiler on a set of .proto files to generate proto buf
# descriptor files. Generates a custom target that may be used in an
# add_dependencies()
#
# Args:
# TARGET_NAME: The name of the generated CMake target.
# DESCRIPTORS_OUT: A variable in which to write the list of names of the
#                  generated descriptor files
# <remaining args>: List of simple names of .proto files to include from the
#                   current source directory. The names should not include
#                   ".proto"
#
# example usage:
#
# cobalt_generate_protobuf_descriptors(generate_report_master_descriptor
#                                      REPORT_MASTER_PROTO_DESCRIPTOR
#                                      report_master)
#
# This will compile the file report_master.proto in the current source dir
# and generate a descriptor for it in the current binary dir. A CMake target
# named generate_report_master_descriptor will be created on which other
# targets may depend. The variable REPORT_MASTER_PROTO_DESCRIPTOR will
# contain the list of strings:
# { <some-path>/report_master.descriptor }
macro(cobalt_generate_protobuf_descriptors TARGET_NAME DESCRIPTORS_OUT)
  set(_protofiles)
  set(_generated_dscrptrs)
  foreach(name ${ARGN})
      list(APPEND _protofiles "${CMAKE_CURRENT_SOURCE_DIR}/${name}.proto")
      list(APPEND _generated_dscrptrs "${CMAKE_CURRENT_BINARY_DIR}/${name}.descriptor")
      set_source_files_properties("${CMAKE_CURRENT_BINARY_DIR}/${name}.descriptor" PROPERTIES GENERATED TRUE)
  endforeach()
  add_custom_command(OUTPUT ${_generated_dscrptrs}
    COMMAND ${PROTOC}
            -I ${CMAKE_SOURCE_DIR}
            -I ${PROTOBUF_INCLUDE_DIR}
            --include_imports
            --include_source_info ${_protofiles}
            --descriptor_set_out ${_generated_dscrptrs}
    DEPENDS ${_protofiles}
    DEPENDS build_external_projects
  )
  add_custom_target(${TARGET_NAME} ALL
    DEPENDS ${_generated_dscrptrs}
  )
  set(${DESCRIPTORS_OUT} ${_generated_dscrptrs})
endmacro()
# Runs the protoc compiler on a set of .proto files to generate go source files.
# Generates a custom target that may be used in an add_dependencies()
#
# Args:
# TARGET_NAME: The name of the generated CMake target.
# SRCS_OUT: A variable in which to write the list of names of the generated
#           go source files
# USE_GRPC: A bool indicating whether or not use the gRPC plugin.
# <remaining args>: List of simple names of .proto files to include from the
#                   current source directory. The names should not include
#                   ".proto"
# example usage:
#
# cobalt_protobuf_generate_go(generate_report_master_pb_go_files
#                             REPORT_MASTER_PB_GO_FILES
#                             true
#                             report_master)
#
# This will compile the file report_master.proto in the current source
# directory and generate a go source file in the appropriate directory
# under ${GO_PROTO_GEN_SRC_DIR}. A CMake target named
# generate_report_master_pb_go_files will be created on which other
# targets may depend. The variable SRCS_OUT will contain the list of strings:
# { <some-path>/report_master.pb.go }
macro(cobalt_protobuf_generate_go TARGET_NAME SRCS_OUT USE_GRPC)
  set(_gen_root_dir ${GO_PROTO_GEN_SRC_DIR})
  string(REPLACE ${CMAKE_BINARY_DIR} ${GO_PROTO_GEN_SRC_DIR} _gen_full_dir ${CMAKE_CURRENT_BINARY_DIR})
  string(COMPARE EQUAL ${CMAKE_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR} is_root_dir)
  if(is_root_dir)
    # What's going on here? Some of our .proto files are in the root directory
    # but use "package cobalt". The Go compiler and the protoc compiler don't
    # play together well for these files. So we treat all of our top-level
    # proto files as if they were in a directory named "cobalt" and this
    # works well with go.
    string(CONCAT _gen_root_dir ${GO_PROTO_GEN_SRC_DIR} "/cobalt")
    set(_gen_full_dir ${_gen_root_dir})
  endif()
  set(_protofiles)
  set(_generated_srcs)
  foreach(name ${ARGN})
      list(APPEND _protofiles "${CMAKE_CURRENT_SOURCE_DIR}/${name}.proto")
      list(APPEND _generated_srcs "${_gen_full_dir}/${name}.pb.go")
      set_source_files_properties("${gen_full}/${name}.pb.go" PROPERTIES GENERATED TRUE)
  endforeach()
  set(_plugin_prefix ",")
  if(${USE_GRPC})
    set(_plugin_prefix "grpc,")
  endif()
  add_custom_command(
    OUTPUT ${_generated_srcs}
    COMMAND ${PROTOC} ${_protofiles}
            -I ${CMAKE_SOURCE_DIR}
            -I ${PROTOBUF_INCLUDE_DIR}
            -I ${CMAKE_SOURCE_DIR}/third_party/go/src
            --go_out=plugins=${_plugin_prefix}Mobservation.proto=cobalt,Mobservation_batch.proto=cobalt,Menvelope.proto=cobalt,Mencrypted_message.proto=cobalt:${_gen_root_dir}
    DEPENDS ${_protofiles}
    DEPENDS build_external_projects
  )
  add_custom_target(${TARGET_NAME}
    DEPENDS ${_generated_srcs}
  )
  set(${SRCS_OUT} ${_generated_srcs})
endmacro()
set(CONFIG_PARSER_BINARY "${CMAKE_BINARY_DIR}/config/config_parser/config_parser")
# Runs the Cobalt config parser on a .yaml file containing Cobalt registry
# for a single project. Generates a C++ header file containing the definition
# of a string containing the base64 encoding of the bytes of a serialized
# CobaltRegistry proto message containing the data specified in the YAML, and
# places the generated constants in a specified namespace.
# This is intended for use in tests.
#
# Args:
# INPUT_YAML: Path to the YAML config file for the test
# OUTFILE: Path to the .h file to produce
# CUSTOMER_ID, PROJECT_ID: These are inserted into the generated CobaltRegistry.
# COBALT_VERSION: The Cobalt version (0 or 1) of the registry.
# NAMESPACE: Namespace of the generated .h file, as a period-separated list.
# STRING_VAR_NAME: The C++ variable name to use in the .h file.
#
# example usage:
#
# set(ENCODER_TEST_CONFIG_H "${CMAKE_CURRENT_BINARY_DIR}/encoder_test_config.h")
# generate_test_config_h(${CMAKE_CURRENT_SOURCE_DIR}/encoder_test_config.yaml
#     ${ENCODER_TEST_CONFIG_H} 1 1 0 "encoder.testing" "cobalt_registry_base64")
#
# This will parse the file "encoder_test_config.yaml" in the current source
# directory and generate a C++ .h file in the current binary out directory
# named "encoder_test_config.h". The header file will contain the definition of
# a variable named |cobalt_registry_base64| of type "const char[]" containing
# the base64 encoded bytes of a serialized CobaltRegistry proto message
# corresponding to the configuration described in the YAML file, and for
# customer_id=1, project_id=1. The contents of the header file will be placed
# in the namespace encoder::testing.
#
# To use this in a test one would #include encoder_test_config.h and then
# type something like the following:
#
# std::unique_ptr<ClientConfig> client_config =
#      ClientConfig::CreateFromCobaltRegistryBase64(cobalt_registry_base64);
macro(generate_test_config_h INPUT_YAML OUTFILE CUSTOMER_ID PROJECT_ID COBALT_VERSION NAMESPACE STRING_VAR_NAME)
  set_source_files_properties(${OUTFILE} PROPERTIES GENERATED TRUE)
  add_custom_command(OUTPUT ${OUTFILE}
    COMMAND ${CONFIG_PARSER_BINARY}
    ARGS -config_file=${INPUT_YAML}
    ARGS -customer_id=${CUSTOMER_ID}
    ARGS -project_id=${PROJECT_ID}
    ARGS -out_format=cpp
    ARGS -namespace=${NAMESPACE}
    ARGS -var_name=${STRING_VAR_NAME}
    ARGS -output_file=${OUTFILE}
    ARGS -skip_validation # We want to be able to have invalid configs in tests.
    ARGS -for_testing # Generate constants for report IDs.
    ARGS -v1_project=${COBALT_VERSION}
    DEPENDS ${INPUT_YAML}
    DEPENDS ${CONFIG_PARSER_BINARY}
    )
endmacro()
macro(generate_cobalt_registry_h)
  set(oneValueArgs CUSTOMER_ID PROJECT_NAME OUTPUT_FILE NAMESPACE)
  cmake_parse_arguments(ARGS "" "${oneValueArgs}" "" ${ARGN})
  if ("${ARGS_OUTPUT_FILE}" STREQUAL "")
    message(FATAL_ERROR "generate_cobalt_registry_h requires OUTPUT_FILE to be specified")
  endif()
  set_source_files_properties(${ARGS_OUTPUT_FILE} PROPERTIES GENERATED TRUE)
  add_custom_command(OUTPUT ${ARGS_OUTPUT_FILE}
    COMMAND ${CONFIG_PARSER_BINARY}
    ARGS -config_dir=${CMAKE_SOURCE_DIR}/third_party/config
    ARGS -customer_id=${ARGS_CUSTOMER_ID}
    ARGS -project_name=${ARGS_PROJECT_NAME}
    ARGS -out_format=cpp
    ARGS -output_file=${ARGS_OUTPUT_FILE}
    ARGS -namespace=${ARGS_NAMESPACE}
    DEPENDS ${CONFIG_PARSER_BINARY}
  )
endmacro()
# Generate the C++ bindings for the Cobalt proto files in the root directory.
# Also compile the generated C++ files into a static library.
cobalt_make_protobuf_cpp_lib(cobalt_proto_lib
                             COBALT_PROTO_HDRS
                             false
                             encrypted_message
                             envelope
                             event
                             observation
                             observation2
                             observation_batch)
# Generate the C++ bindings for some server-side-only proto files.
# These are protos that are not used on the client at all but nevertheless
# are stored in the Cobalt repo so that they are open-sourced. Here we
# build them just to ensure they are syntactically correct.
cobalt_make_protobuf_cpp_lib(cobalt_server_side_proto_lib
                             COBALT_SERVER_SIDE_PROTO_HDRS
                             false
                             report_row)
add_custom_target(BUILD_SERVER_SIDE_PROTOS ALL
    DEPENDS cobalt_server_side_proto_lib
)
# Generate the go bindings for the Cobalt proto files in the root directory.
cobalt_protobuf_generate_go(generate_cobalt_pb_go_files
                            COBALT_PB_GO_FILES
                            false
                            encrypted_message
                            envelope
                            observation
                            observation_batch)
# Analagous targets to the two above also appear in the config directory for
# the config protos. But the variables that are generated there are not
# available in other sub-directories of the root directory. So we variables
# here that contain the same content.
set(CONFIG_PROTO_HDRS
    "${CMAKE_BINARY_DIR}/config/encodings.pb.h"
    "${CMAKE_BINARY_DIR}/config/metrics.pb.h"
    "${CMAKE_BINARY_DIR}/config/report_configs.pb.h"
    "${CMAKE_BINARY_DIR}/config/report_definition.pb.h")
set(CONFIG_PB_GO_FILES
    "${GO_PROTO_GEN_SRC_DIR}/config/cobalt_registry.pb.go"
    "${GO_PROTO_GEN_SRC_DIR}/config/encodings.pb.go"
    "${GO_PROTO_GEN_SRC_DIR}/config/metrics.pb.go"
    "${GO_PROTO_GEN_SRC_DIR}/config/report_configs.pb.go"
    "${GO_PROTO_GEN_SRC_DIR}/config/report_definition.pb.go")
macro(declare_proto_files_are_generated)
  set_source_files_properties(${COBALT_PROTO_HDRS} PROPERTIES GENERATED TRUE)
  set_source_files_properties(${CONFIG_PROTO_HDRS} PROPERTIES GENERATED TRUE)
  set_source_files_properties(${COBALT_PB_GO_FILES} PROPERTIES GENERATED TRUE)
  set_source_files_properties(${CONFIG_PB_GO_FILES} PROPERTIES GENERATED TRUE)
endmacro()
macro(add_cobalt_test_dependencies target_name test_dir)
  target_link_libraries(${target_name}
    gtest gtest_main
  )
  add_cobalt_dependencies(${target_name})
  set_target_properties(${target_name}
      PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${test_dir})
endmacro()
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/third_party/abseil-cpp)
include_directories(${CMAKE_BINARY_DIR}/third_party/tink/include)
# //third_party/clearcut is a special case. It is code that we own but
# but we keep it under //third_party.
add_subdirectory(third_party/clearcut)
# Generate the C++ bindings for the Clearcut extension proto.
# Also compile the generated C++ files into a static library.
# This depends on third_party/clearcut
set_source_files_properties(third_party/clearcut/clearcut.pb.h PROPERTIES GENERATED TRUE)
cobalt_make_protobuf_cpp_lib(clearcut_extensions_proto_lib
                             CLEARCUT_EXTENSIONS_PROTO_HDRS
                             false
                             clearcut_extensions)
target_link_libraries(clearcut_extensions_proto_lib
                      clearcut)
# Project directories
add_subdirectory(algorithms)
add_subdirectory(analyzer)
add_subdirectory(client/collection)
add_subdirectory(config)
add_subdirectory(encoder)
add_subdirectory(end_to_end_tests)
add_subdirectory(logger)
add_subdirectory(shuffler)
add_subdirectory(tools)
add_subdirectory(util)
# Turn off extra warnings on third_party code we don't control.
add_compile_options(-Wno-sign-compare
                    -Wno-ignored-qualifiers
                    -Wno-unused-local-typedef
                    -Wno-deprecated-declarations
                    -Wno-unused-private-field)
# Third-party directories that we include using the simpler add_subdirectory method
add_subdirectory(third_party/abseil-cpp)
add_subdirectory(third_party/statusor)
add_subdirectory(third_party/rapidjson)
add_subdirectory(third_party/tink)
```
