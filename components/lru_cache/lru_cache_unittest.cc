//
// Created by gh on 18-9-15.
//
#include "components/lru_cache/lru_cache.h"
#include <stdlib.h>
#include <unistd.h>
#include <iostream>

#include <catch2/catch_test_macros.hpp>

TEST_CASE("lru.base", "[base lru function case]") {
  component::LRUCache<std::string, int64_t> cache(5);
  REQUIRE(cache.size() == 0);
  REQUIRE(cache.Exists("name") == false);

  cache.Add("pear", 10);
  cache.Add("race", 20);
  cache.Add("apple", 100);
  cache.Add("banana", 50);
  REQUIRE(cache.size() == 4);
  cache.Add("banana", 60);
  REQUIRE(cache.size() == 4);

  REQUIRE(cache.Exists("banana"));
  cache.Add("name", 1000);
  REQUIRE(cache.size() == 5);

  // 淘汰第一个添加的pear
  cache.Add("name2", 10);
  REQUIRE(!cache.Exists("pear"));
  REQUIRE(cache.size() == 5);

  int64_t value;
  REQUIRE(cache.Get("name", &value) == true);
  REQUIRE(value == 1000);

  REQUIRE(cache.Get("pear", &value) == false);

  REQUIRE(cache.Get("name2", &value) == true);
  REQUIRE(value == 10);
}
