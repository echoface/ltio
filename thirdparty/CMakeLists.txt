set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-sign-compare")

set(BUILD_TESTING OFF CACHE BOOL "enable glog uinittest functionality" FORCE)

ADD_SUBDIRECTORY(resp)
ADD_SUBDIRECTORY(libcoro)

#set_source_files_properties(md5/md5.c PROPERTIES LANGUAGE CXX)
#set_source_files_properties(urlsafe64/urlsafe64.c PROPERTIES LANGUAGE CXX)
#set_source_files_properties(http_parser/http_parser.c PROPERTIES LANGUAGE CXX)
#set_source_files_properties(timeout/timeout.c PROPERTIES LANGUAGE CXX)
#set_source_files_properties(timeout/timeout-bitops.c PROPERTIES LANGUAGE CXX)

ADD_LIBRARY(3rd STATIC
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
