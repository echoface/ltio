# system wide deps
#find_package(PROFILER REQUIRED)
#list(APPEND LtIO_LINKER_LIBS PUBLIC ${PROFILER_LIBRARIES})

find_package(ZLIB REQUIRED)
find_package(Unwind REQUIRED)
find_package(Threads REQUIRED)
find_package(Tcmalloc REQUIRED)

# thirdparty deps
CPMAddPackage("gh:fmtlib/fmt#10.1.0")
CPMAddPackage("gh:nlohmann/json@3.10.5")
CPMAddPackage("gh:catchorg/Catch2@3.2.1")
CPMAddPackage("gh:google/glog@0.4.0")
CPMAddPackage("gh:gflags/gflags@2.2.2")

list(APPEND LtIO_LINKER_LIBS PUBLIC ${ZLIB_LIBRARIES})
list(APPEND LtIO_LINKER_LIBS PUBLIC ${Tcmalloc_LIBRARY})
list(APPEND LtIO_LINKER_LIBS PUBLIC ${LIBUNWIND_LIBRARIES})
list(APPEND LtIO_LINKER_LIBS PRIVATE ${CMAKE_THREAD_LIBS_INIT})
list(APPEND LtIO_LINKER_LIBS PUBLIC fmt::fmt)
list(APPEND LtIO_LINKER_LIBS PUBLIC glog::glog)
list(APPEND LtIO_LINKER_LIBS PUBLIC gflags::gflags)

if (LTIO_WITH_OPENSSL)
  find_package(OpenSSL REQUIRED)
  list(APPEND LtIO_LINKER_LIBS PUBLIC ${OPENSSL_LIBRARIES})
endif()

if (LTIO_WITH_HTTP2)
  if(LTIO_USE_SYS_NGHTTP2)
    find_package(NGHTTP2 REQUIRED)
    list(APPEND LtIO_LINKER_LIBS PUBLIC ${NGHTTP2_LIBRARIES})
  else()
    CPMAddPackage("gh:nghttp2/nghttp2@1.44.0")
    list(APPEND LtIO_LINKER_LIBS PUBLIC nghttp2)
  endif()
endif()

# ---[ ccache
find_program(CCACHE_FOUND ccache)
if(CCACHE_FOUND)
  set_property(GLOBAL PROPERTY RULE_LAUNCH_LINK ccache)
  set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ccache)
endif(CCACHE_FOUND)
