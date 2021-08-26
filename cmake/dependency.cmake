find_package(Tcmalloc REQUIRED)
list(APPEND LtIO_LINKER_LIBS PUBLIC ${Tcmalloc_LIBRARY})

## ---[ Google-gflags
include("cmake/External/glog_gflags.cmake")
list(APPEND LtIO_LINKER_LIBS PUBLIC glog::glog)
list(APPEND LtIO_LINKER_LIBS PUBLIC gflags::gflags)
list(APPEND LtIO_INCLUDE_DIRS PUBLIC ${GLOG_INCLUDE_DIRS})
list(APPEND LtIO_INCLUDE_DIRS PUBLIC ${GFLAGS_INCLUDE_DIRS})
list(APPEND LtIO_INCLUDE_DIRS
  PUBLIC
  $<TARGET_PROPERTY:fcontext,INTERFACE_INCLUDE_DIRECTORIES>
  )

find_package(Unwind REQUIRED)
list(APPEND LtIO_LINKER_LIBS PUBLIC ${LIBUNWIND_LIBRARIES})

find_package(ZLIB REQUIRED)
list(APPEND LtIO_LINKER_LIBS PUBLIC ${ZLIB_LIBRARIES})

# ---[ Threads
find_package(Threads REQUIRED)
list(APPEND LtIO_LINKER_LIBS PRIVATE ${CMAKE_THREAD_LIBS_INIT})

#find_package(PROFILER REQUIRED)
#list(APPEND LtIO_LINKER_LIBS PUBLIC ${PROFILER_LIBRARIES})
list(APPEND LtIO_LINKER_LIBS PUBLIC llhttp::llhttp)

if (LTIO_WITH_OPENSSL)
  find_package(OpenSSL REQUIRED)
  list(APPEND LtIO_LINKER_LIBS PUBLIC ${OPENSSL_LIBRARIES})
  list(APPEND LtIO_INCLUDE_DIRS PUBLIC ${OPENSSL_INCLUDE_DIR})
endif()

if (LTIO_WITH_HTTP2)
  if(LTIO_USE_SYS_NGHTTP2)
    find_package(NGHTTP2 REQUIRED)
    list(APPEND LtIO_LINKER_LIBS PUBLIC ${NGHTTP2_LIBRARIES})
    list(APPEND LtIO_INCLUDE_DIRS PUBLIC ${NGHTTP2_INCLUDE_DIRS})
  else()
    include(FetchContent)
    FetchContent_Declare(nghttp2
      GIT_REPOSITORY https://github.com/nghttp2/nghttp2.git
      GIT_TAG        v1.44.0
      #GIT_SUBMODULES
    )
    FetchContent_GetProperties(nghttp2)
    if(NOT nghttp2_POPULATED)
      set(ENABLE_LIB_ONLY ON CACHE INTERNAL "")
      set(ENABLE_STATIC_LIB OFF CACHE INTERNAL "")
      FetchContent_Populate(nghttp2) # download and extract

      message(STATUS "nghttp2 source dir: ${nghttp2_SOURCE_DIR}")
      message(STATUS "nghttp2 binary dir: ${nghttp2_BINARY_DIR}")
      add_subdirectory(${nghttp2_SOURCE_DIR} ${nghttp2_BINARY_DIR})
    endif()

    list(APPEND LtIO_LINKER_LIBS PUBLIC nghttp2)
    list(APPEND LtIO_INCLUDE_DIRS PUBLIC ${nghttp2_SOURCE_DIR}/lib/includes)
    # <nghttp2/nghttp2ver.h>
    list(APPEND LtIO_INCLUDE_DIRS PUBLIC ${nghttp2_BINARY_DIR}/lib/includes)
  endif()
endif()

list(REMOVE_DUPLICATES LtIO_INCLUDE_DIRS)

# ---[ ccache
find_program(CCACHE_FOUND ccache)
if(CCACHE_FOUND)
  set_property(GLOBAL PROPERTY RULE_LAUNCH_LINK ccache)
  set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ccache)
endif(CCACHE_FOUND)

