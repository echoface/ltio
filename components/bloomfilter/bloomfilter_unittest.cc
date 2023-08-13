#include <stdlib.h>
#include <unistd.h>
#include <iostream>

#include <catch2/catch_test_macros.hpp>

#include "components/bloomfilter/bloomfilter.h"

TEST_CASE("base", "[Add New Item To BloomFilter]") {
  component::BloomFilter filter(1000000, 0.00001);

  for (uint64_t i = 0; i < 1000000; i++) {
    filter.Set(&i, sizeof(uint64_t));
    REQUIRE(filter.IsSet(&i, sizeof(uint64_t)));
  }
}

TEST_CASE("bloom_filter.falserate", "[false case]") {
  component::BloomFilter filter(1000000, 0.00001);

  for (uint64_t i = 0; i < 1000000; i++) {
    filter.Set(&i, sizeof(uint64_t));
  }

  uint64_t false_count = 0;
  for (uint64_t i = 5000000; i < 10000000; i++) {
    if (filter.IsSet(&i, sizeof(uint64_t))) {
      false_count++;
    }
  }

  REQUIRE(false_count < 50);
}
