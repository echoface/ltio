#include <vector>
#include <iostream>

#include <utils/string/str_utils.h>

#include <catch/catch.hpp>

TEST_CASE("string_utils", "[]") {

  std::vector<std::string> test_strs = {"742.16", ".123", "123.", "abc"};
  std::vector<double> test_results = {742.16, 0.123, 123.0, 0};
  for (size_t idx = 0; idx < test_strs.size(); idx++) {
    double num_double = base::Str::Parse<double>(test_strs[idx]);
    CHECK(num_double == test_results[idx]);
  }

  std::vector<std::string> nums = {"1", "", "118", "205", "199"};
  CHECK(base::Str::Join(nums, "-") == "1--118-205-199");

  std::string ok = "ok";
  bool ok_bool = base::Str::Parse<bool>(ok);
  CHECK(!ok_bool);

  std::string none_true_str = "abc";
  ok_bool = base::Str::Parse<bool>(none_true_str);
  CHECK(!ok_bool);
}
