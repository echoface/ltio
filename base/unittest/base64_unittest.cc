#include <unistd.h>
#include <iostream>
#include <unistd.h>
#include <stdlib.h>
#include <atomic>

#include "glog/logging.h"
#include <time/time_utils.h>
#include <utils/base64/base64.hpp>

#include <catch/catch.hpp>

/*
TEST_CASE("base64.base", "[base64 basic test]") {
   REQUIRE("U2VuZCByZWluZm9yY2VtZW50cw==" == base::base64_encode("Send reinforcements"));
   REQUIRE("Send reinforcements" == base::base64_decode("U2VuZCByZWluZm9yY2VtZW50cw=="));
   REQUIRE("VGhpcyBpcyBsaW5lIG9uZQpUaGlzIGlzIGxpbmUgdHdvClRoaXMgaXMgbGlu\nZSB0aHJlZQpBbmQgc28gb24uLi4K" == 
           base::base64_encode("This is line one\nThis is line two\nThis is line three\nAnd so on..."));
   REQUIRE("This is line one\nThis is line two\nThis is line three\nAnd so on..." == 
           base::base64_decode("VGhpcyBpcyBsaW5lIG9uZQpUaGlzIGlzIGxpbmUgdHdvClRoaXMgaXMgbGluZSB0aHJlZQpBbmQgc28gb24uLi4K"));
}

TEST_CASE("base64.encode", "[base64 encode test]") {
    REQUIRE("" == base::base64_encode(""));
    REQUIRE("AA==" == base::base64_encode("\0"));
    REQUIRE("AAA=" == base::base64_encode("\0\0"));
    REQUIRE("AAAA" == base::base64_encode("\0\0\0"));
    REQUIRE("/w==" == base::base64_encode("\377"));
    REQUIRE("//8=" == base::base64_encode("\377\377"));
    REQUIRE("////" == base::base64_encode("\377\377\377"));
    REQUIRE("/+8=" == base::base64_encode("\xff\xef"));
}

TEST_CASE("base64.decode", "[base64 decode test]") {
    REQUIRE("" == base::base64_decode(""));
    REQUIRE("\0" == base::base64_decode("AA=="));
    REQUIRE("\0\0" == base::base64_decode("AAA="));
    REQUIRE("\0\0\0" == base::base64_decode("AAAA"));
    REQUIRE("\377" == base::base64_decode("/w=="));
    REQUIRE("\377\377" == base::base64_decode("//8="));
    REQUIRE("\377\377\377" == base::base64_decode("////"));
    REQUIRE("\xff\xef" == base::base64_decode("/+8="));
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
    REQUIRE("", Base64.urlsafe_decode64(""))
    REQUIRE("\0", Base64.urlsafe_decode64("AA=="))
    REQUIRE("\0\0", Base64.urlsafe_decode64("AAA="))
    REQUIRE("\0\0\0", Base64.urlsafe_decode64("AAAA"))
    REQUIRE("\377", Base64.urlsafe_decode64("_w=="))
    REQUIRE("\377\377", Base64.urlsafe_decode64("__8="))
    REQUIRE("\377\377\377", Base64.urlsafe_decode64("____"))
    REQUIRE("\xff\xef", Base64.urlsafe_decode64("_+8="))
  end
end
*/
