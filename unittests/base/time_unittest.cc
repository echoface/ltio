#include <thirdparty/catch/catch.hpp>

#include <base/time/time.h>
#include <base/time/time_ticks.h>
#include <unistd.h>
#include <iostream>

TEST_CASE("time_delta", "[test time delta]") {
  auto delta = base::TimeDelta::FromSeconds(10);
  REQUIRE(delta.ToHz() == 0.1);
  REQUIRE(delta.InMilliseconds() == 10000);

  REQUIRE_FALSE(delta.is_inf());
  REQUIRE_FALSE(delta.is_zero());
  REQUIRE_FALSE(delta.is_max());
  REQUIRE_FALSE(delta.is_min());

  base::TimeTicks now = base::TimeTicks::Now();
  sleep(1);
  delta = base::TimeTicks::Now() - now;
  REQUIRE(delta.InMilliseconds() >= 1000);
  REQUIRE(delta.InMilliseconds() <= 1010);

  std::cout << "sizeof(TimeDelta):" << sizeof(base::TimeDelta) << std::endl;
  std::cout << "sizeof(TimeTicks):" << sizeof(base::TimeTicks) << std::endl;
  std::cout << "sizeof(Time):" << sizeof(base::Time) << std::endl;
}
