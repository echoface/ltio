set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-sign-compare")

target_sources(ltio
    PRIVATE
    #hash
    hash/md5.c
    hash/crc.cpp
    hash/sha1.cpp
    hash/murmurhash3.cpp

    # timeout wheel
    timeout/timeout.h
    timeout/timeout.c

    urlsafe64/urlsafe64.c
    tinyxml2/tinyxml2.cpp
    url-parser/url_parser.c
    )
