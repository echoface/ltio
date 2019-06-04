#include <vector>
#include <iostream>

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
  CHECK(!ok_bool);

  std::string none_true_str = "abc";
  ok_bool = base::Str::Parse<bool>(none_true_str);
  CHECK(!ok_bool);
}
