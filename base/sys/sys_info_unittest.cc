#include "base/sys/sys_info.h"

#include <stdint.h>
#include <iostream>

#include "base/build_config.h"
#include "base/utils/sys_error.h"
#include <catch2/catch_test_macros.hpp>

TEST_CASE("sysinfo.getncpu", "[get system cpu count]") {
  std::cout << "cpu count:" << base::GetNCPU() << std::endl;
}

TEST_CASE("sysinfo.meminfo", "[get system meminfo]") {
  base::meminfo_t info;
  REQUIRE(base::GetMemoryInfo(&info) == 0);
  std::cout << info.total << ", " << info.free << std::endl;
  std::cout << "free:" << base::PrettyBytes(info.free * 1024) << std::endl;
  std::cout << "total:" << base::PrettyBytes(info.total * 1024) << std::endl;
}
