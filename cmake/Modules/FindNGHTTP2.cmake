# LTIO HTTP2 support

include(FindPackageHandleStandardArgs)

find_path(NGHTTP2_INCLUDE_DIR "nghttp2/nghttp2.h")

find_library(NGHTTP2_LIBRARY NAMES nghttp2)

find_package_handle_standard_args(NGHTTP2
    FOUND_VAR
      NGHTTP2_FOUND
    REQUIRED_VARS
      NGHTTP2_LIBRARY
      NGHTTP2_INCLUDE_DIR
)

if (NGHTTP2_FOUND)
  set(NGHTTP2_INCLUDE_DIRS ${NGHTTP2_INCLUDE_DIR})
  set(NGHTTP2_LIBRARIES ${NGHTTP2_LIBRARY})
  mark_as_advanced(NGHTTP2_INCLUDE_DIRS NGHTTP2_LIBRARIES)
else() 
  message(STATUS "package nghttp2 NOT FOUND")
  if(NGHTTP2_FIND_REQUIRED)
    message(FATAL_ERROR "Could NOT find nghttp2 library")
  endif()
endif()
