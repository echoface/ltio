#include <stdlib.h>
#include <unistd.h>
#include <atomic>
#include <iostream>

#include <base/time/time_utils.h>
#include <base/utils/base64/base64.hpp>
#include "glog/logging.h"

#include <catch2/catch_test_macros.hpp>

CATCH_TEST_CASE("base64.empty", "[base64 empty test]") {
  CATCH_REQUIRE_FALSE(false);
}
/*
CATCH_TEST_CASE("base64.base", "[base64 basic test]") {
   REQUIRE("U2VuZCByZWluZm9yY2VtZW50cw==" == base::base64_encode("Send
reinforcements")); REQUIRE("Send reinforcements" ==
base::base64_decode("U2VuZCByZWluZm9yY2VtZW50cw=="));
   REQUIRE("VGhpcyBpcyBsaW5lIG9uZQpUaGlzIGlzIGxpbmUgdHdvClRoaXMgaXMgbGlu\nZSB0aHJlZQpBbmQgc28gb24uLi4K"
== base::base64_encode("This is line one\nThis is line two\nThis is line
three\nAnd so on...")); REQUIRE("This is line one\nThis is line two\nThis is
line three\nAnd so on..." ==
           base::base64_decode("VGhpcyBpcyBsaW5lIG9uZQpUaGlzIGlzIGxpbmUgdHdvClRoaXMgaXMgbGluZSB0aHJlZQpBbmQgc28gb24uLi4K"));
}

CATCH_TEST_CASE("base64.encode", "[base64 encode test]") {
    CATCH_REQUIRE("" == base::base64_encode(""));
    CATCH_REQUIRE("AA==" == base::base64_encode("\0"));
    CATCH_REQUIRE("AAA=" == base::base64_encode("\0\0"));
    CATCH_REQUIRE("AAAA" == base::base64_encode("\0\0\0"));
    CATCH_REQUIRE("/w==" == base::base64_encode("\377"));
    CATCH_REQUIRE("//8=" == base::base64_encode("\377\377"));
    CATCH_REQUIRE("////" == base::base64_encode("\377\377\377"));
    CATCH_REQUIRE("/+8=" == base::base64_encode("\xff\xef"));
}

CATCH_TEST_CASE("base64.decode", "[base64 decode test]") {
    CATCH_REQUIRE("" == base::base64_decode(""));
    CATCH_REQUIRE("\0" == base::base64_decode("AA=="));
    CATCH_REQUIRE("\0\0" == base::base64_decode("AAA="));
    CATCH_REQUIRE("\0\0\0" == base::base64_decode("AAAA"));
    CATCH_REQUIRE("\377" == base::base64_decode("/w=="));
    CATCH_REQUIRE("\377\377" == base::base64_decode("//8="));
    CATCH_REQUIRE("\377\377\377" == base::base64_decode("////"));
    CATCH_REQUIRE("\xff\xef" == base::base64_decode("/+8="));
}
*/
/*
    assert_raise(ArgumentError) { base::base64_decode("^") }
    assert_raise(ArgumentError) { base::base64_decode("A") }
    assert_raise(ArgumentError) { base::base64_decode("A^") }
    assert_raise(ArgumentError) { base::base64_decode("AA") }
    assert_raise(ArgumentError) { base::base64_decode("AA=") }
    assert_raise(ArgumentError) { base::base64_decode("AA===") }
    assert_raise(ArgumentError) { base::base64_decode("AA=x") }
    assert_raise(ArgumentError) { base::base64_decode("AAA") }
    assert_raise(ArgumentError) { base::base64_decode("AAA^") }
    assert_raise(ArgumentError) { base::base64_decode("AB==") }
    assert_raise(ArgumentError) { base::base64_decode("AAB=") }
  end

  def test_urlsafe_decode64
    CATCH_REQUIRE("", Base64.urlsafe_decode64(""))
    CATCH_REQUIRE("\0", Base64.urlsafe_decode64("AA=="))
    CATCH_REQUIRE("\0\0", Base64.urlsafe_decode64("AAA="))
    CATCH_REQUIRE("\0\0\0", Base64.urlsafe_decode64("AAAA"))
    CATCH_REQUIRE("\377", Base64.urlsafe_decode64("_w=="))
    CATCH_REQUIRE("\377\377", Base64.urlsafe_decode64("__8="))
    CATCH_REQUIRE("\377\377\377", Base64.urlsafe_decode64("____"))
    CATCH_REQUIRE("\xff\xef", Base64.urlsafe_decode64("_+8="))
  end
end
*/
