#include <stdlib.h>
#include <unistd.h>
#include <atomic>
#include <iostream>

#include "glog/logging.h"
#include <catch2/catch_test_macros.hpp>

CATCH_TEST_CASE("crypto.empty", "[emptycheck]") {
    CATCH_REQUIRE_FALSE(false);
}

#ifdef LTIO_HAVE_SSL
#include <base/crypto/lt_ssl.h>

CATCH_TEST_CASE("crypto.ctx", "[run crypto ctx test]") {
    REQUIRE_FALSE(false);
}
#endif
