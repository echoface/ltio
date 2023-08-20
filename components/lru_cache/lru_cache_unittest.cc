//
// Created by gh on 18-9-15.
//
#include "components/lru_cache/lru_cache.h"
#include <stdlib.h>
#include <unistd.h>
#include <iostream>

#include <catch2/catch_test_macros.hpp>

CATCH_TEST_CASE("lru.base", "[base lru function case]") {
  component::LRUCache<std::string, int64_t> cache(5);
  CATCH_REQUIRE(cache.size() == 0);
  CATCH_REQUIRE(cache.Exists("name") == false);

  cache.Add("pear", 10);
  cache.Add("race", 20);
  cache.Add("apple", 100);
  cache.Add("banana", 50);
  CATCH_REQUIRE(cache.size() == 4);
  cache.Add("banana", 60);
  CATCH_REQUIRE(cache.size() == 4);

  CATCH_REQUIRE(cache.Exists("banana"));
  cache.Add("name", 1000);
  CATCH_REQUIRE(cache.size() == 5);

  // 淘汰第一个添加的pear
  cache.Add("name2", 10);
  CATCH_REQUIRE(!cache.Exists("pear"));
  CATCH_REQUIRE(cache.size() == 5);

  int64_t value;
  CATCH_REQUIRE(cache.Get("name", &value) == true);
  CATCH_REQUIRE(value == 1000);

  CATCH_REQUIRE(cache.Get("pear", &value) == false);

  CATCH_REQUIRE(cache.Get("name2", &value) == true);
  CATCH_REQUIRE(value == 10);
}
