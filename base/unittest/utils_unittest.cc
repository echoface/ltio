#include <vector>
#include <iostream>

#include <url_string_utils.h>
#include <utils/string/str_utils.h>

#include <catch/catch.hpp>

TEST_CASE("string_utils", "[]") {

  std::string str_double("742.16");
  double num_double = base::Str::Parse<double>(str_double);
  CHECK(num_double == 742.16);

  std::vector<std::string> nums = {"1", "", "118", "205", "199"};
  CHECK(base::Str::Join(nums, "-") == "1--118-205-199");

  std::string ok = "ok";
  bool ok_bool = base::Str::Parse<bool>(ok);
  CHECK(ok_bool);

  std::string none_true_str = "abc";
  ok_bool = base::Str::Parse<bool>(none_true_str);
  CHECK(!ok_bool);
}

TEST_CASE("url.host_resolve", "[host resolve test]") {
  std::string host_ip;
  net::url::HostResolve("g.test.amnetapi.com", host_ip);
  std::cout << "result:" << host_ip << std::endl;
}
