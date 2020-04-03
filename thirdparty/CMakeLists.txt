set(CMAKE_POSITION_INDEPENDENT_CODE ON)

ADD_SUBDIRECTORY(resp)
ADD_SUBDIRECTORY(libcoro)
ADD_SUBDIRECTORY(gflags)

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
