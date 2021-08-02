#include <base/closure/closure_task.h>
#include <base/utils/string/str_utils.h>
#include <iostream>
#include <thirdparty/catch/catch.hpp>
#include <vector>

TEST_CASE("string_utils", "[]") {
  std::vector<std::string> test_strs = {"742.16", ".123", "123.", "abc"};
  std::vector<double> test_results = {742.16, 0.123, 123.0, 0};
  for (size_t idx = 0; idx < test_strs.size(); idx++) {
    double num_double = base::StrUtil::Parse<double>(test_strs[idx]);
    CHECK(num_double == test_results[idx]);
  }

  std::vector<std::string> nums = {"1", "", "118", "205", "199"};
  CHECK(base::StrUtil::Join(nums, "-") == "1--118-205-199");

  std::string ok = "ok";
  bool ok_bool = base::StrUtil::Parse<bool>(ok);
  CHECK(!ok_bool);

  std::string none_true_str = "abc";
  ok_bool = base::StrUtil::Parse<bool>(none_true_str);
  CHECK(!ok_bool);
}

TEST_CASE("multi lambda capture list", "[ task run]") {
  int a = 0;
  int b = 0;
  auto task1 = NewClosure([&]() { LOG(INFO) << a << ", b:" << b; });
  auto task2 = NewClosure([a, b]() { LOG(INFO) << a << ", b:" << b; });
}
