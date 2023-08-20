#include <catch2/catch_test_macros.hpp>

#include <base/time/time.h>
#include <base/time/time_ticks.h>
#include <unistd.h>
#include <iostream>
#include "base/time/time_utils.h"

CATCH_TEST_CASE("time_delta", "[test time delta]") {
  auto delta = base::TimeDelta::FromSeconds(10);
  CATCH_REQUIRE(delta.ToHz() == 0.1);
  CATCH_REQUIRE(delta.InMilliseconds() == 10000);

  CATCH_REQUIRE_FALSE(delta.is_inf());
  CATCH_REQUIRE_FALSE(delta.is_zero());
  CATCH_REQUIRE_FALSE(delta.is_max());
  CATCH_REQUIRE_FALSE(delta.is_min());

  int64_t ms = base::time_ms();
  base::TimeTicks now = base::TimeTicks::Now();
  sleep(1);
  delta = base::TimeTicks::Now() - now;
  int64_t msdelta = base::time_ms() - ms;

  int64_t diff = delta.InMilliseconds() - msdelta;
  CATCH_REQUIRE(diff <= 1);

  std::cout << "sizeof(TimeDelta):" << sizeof(base::TimeDelta) << std::endl;
  std::cout << "sizeof(TimeTicks):" << sizeof(base::TimeTicks) << std::endl;
  std::cout << "sizeof(Time):" << sizeof(base::Time) << std::endl;
}
