#include <stdlib.h>
#include <unistd.h>
#include <atomic>
#include <iostream>

#include "glog/logging.h"
#include <thirdparty/catch/catch.hpp>

#ifdef LTIO_HAVE_SSL
#include <base/crypto/lt_ssl.h>

TEST_CASE("crypto.ctx", "[run crypto ctx test]") {
}
#endif
