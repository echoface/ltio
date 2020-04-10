set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-sign-compare")

ADD_SUBDIRECTORY(resp)
ADD_SUBDIRECTORY(libcoro)
ADD_SUBDIRECTORY(hat-trie)

SET(LT_3RD_SRSC
  #hash
  md5/md5.c
  crc/crc.cpp
  murmurhash/MurmurHash3.cpp

  # timeout wheel
  timeout/timeout.c
  timeout/timeout-bitops.c

  urlsafe64/urlsafe64.c
  tinyxml2/tinyxml2.cpp
  http_parser/http_parser.c
  )

ADD_LIBRARY(lt3rd STATIC
  ${LT_3RD_SRSC}
)
ltio_default_properties(lt3rd)
