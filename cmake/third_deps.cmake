# system wide deps

find_package(ZLIB REQUIRED)
find_package(Unwind REQUIRED)
find_package(Threads REQUIRED)
find_package(Tcmalloc REQUIRED)
#find_package(PROFILER REQUIRED)

list(APPEND LtIO_LINKER_LIBS PRIVATE ${ZLIB_LIBRARIES})
list(APPEND LtIO_LINKER_LIBS PRIVATE ${Tcmalloc_LIBRARY})
list(APPEND LtIO_LINKER_LIBS PRIVATE ${LIBUNWIND_LIBRARIES})
list(APPEND LtIO_LINKER_LIBS PRIVATE ${CMAKE_THREAD_LIBS_INIT})

# source code deps
ADD_SUBDIRECTORY(thirdparty/resp)

ADD_SUBDIRECTORY(thirdparty/hat-trie)

ADD_SUBDIRECTORY(thirdparty/fcontext)
list(APPEND LtIO_LINKER_LIBS PRIVATE fcontext)

# thirdparty deps
CPMAddPackage("gh:fmtlib/fmt#10.1.0")
CPMAddPackage("gh:nlohmann/json@3.10.5")
CPMAddPackage("gh:gflags/gflags@2.2.2")
CPMAddPackage("gh:cameron314/concurrentqueue@1.0.4")

#CPMAddPackage("gh:google/glog@0.4.0")
CPMAddPackage(
    NAME glog
    VERSION 0.4.0
    OPTIONS "WITH_GFLAGS NO" # disable warning about gflags not found
    GITHUB_REPOSITORY google/glog
    )

#CPMAddPackage("gh:catchorg/Catch2@3.2.1")
CPMAddPackage(
    NAME Catch2
    VERSION 3.2.1
    OPTIONS "CATCH_CONFIG_PREFIX_ALL" # fix `CHECK` conflict with glog
    GITHUB_REPOSITORY catchorg/Catch2
    )

CPMAddPackage(
    NAME llhttp
    VERSION v9.0.0
    OPTIONS "BUILD_STATIC_LIBS ON"
    URL https://github.com/nodejs/llhttp/archive/refs/tags/release/v9.0.0.tar.gz
    )


list(APPEND LtIO_LINKER_LIBS PUBLIC fmt::fmt)
list(APPEND LtIO_LINKER_LIBS PUBLIC glog::glog)
list(APPEND LtIO_LINKER_LIBS PUBLIC gflags::gflags)
list(APPEND LtIO_LINKER_LIBS PUBLIC llhttp::llhttp)
list(APPEND LtIO_LINKER_LIBS PUBLIC concurrentqueue)
list(APPEND LtIO_LINKER_LIBS PUBLIC nlohmann_json::nlohmann_json)

if (LTIO_WITH_OPENSSL)
  find_package(OpenSSL REQUIRED)
  list(APPEND LtIO_LINKER_LIBS PUBLIC ${OPENSSL_LIBRARIES})
endif()

if (LTIO_WITH_HTTP2)
  if(LTIO_USE_SYS_NGHTTP2)
    find_package(NGHTTP2 REQUIRED)
  else()
    CPMAddPackage("gh:nghttp2/nghttp2@1.55.1")
  endif()

  list(APPEND LtIO_LINKER_LIBS PUBLIC nghttp2)
endif()

message("dependency lib:${LtIO_LINKER_LIBS}")

# ---[ ccache
find_program(CCACHE_FOUND ccache)
if(CCACHE_FOUND)
  set_property(GLOBAL PROPERTY RULE_LAUNCH_LINK ccache)
  set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ccache)
endif(CCACHE_FOUND)

