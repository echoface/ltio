set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-sign-compare")

set(BUILD_TESTING OFF CACHE BOOL "enable glog uinittest functionality" FORCE)

ADD_SUBDIRECTORY(resp)
ADD_SUBDIRECTORY(libcoro)

#ADD_SUBDIRECTORY(gflags)
#option (WITH_GFLAGS "Use gflags" OFF)
##set(CMAKE_SKIP_INSTALL_RULES ON CACHE BOOL "disable glog install action" FORCE)
#ADD_SUBDIRECTORY(glog)

SET(LT_THIRDPARTY_SRC
  urlsafe64/urlsafe64.c
  tinyxml2/tinyxml2.cpp
  http_parser/http_parser.c

  #hash
  md5/md5.c
  crc/crc.cpp
  murmurhash/MurmurHash3.cpp

  # timeout wheel
  timeout/timeout.c
  timeout/timeout-bitops.c
)

ADD_LIBRARY(3rd STATIC
  ${LT_THIRDPARTY_SRC}
)
