#include <stdlib.h>
#include <unistd.h>
#include <atomic>
#include <iostream>
#include <thread>

#include <glog/logging.h>
#include <base/time/time_utils.h>
#include <catch2/catch_test_macros.hpp>

#include <components/utils/data_accumulator.h>

TEST_CASE("data.accumulator", "[acc data]") {
  typedef component::AccumulatorCollector<int> IntCollector;
  IntCollector collector(1000);
  collector.SetHandler([&](const IntCollector::AccumulationMap& all) {
    for (auto pair : all) {
      LOG(INFO) << pair.first << " : " << pair.second;
    }
  });
  collector.Start();

  std::vector<std::thread*> threads;
  for (uint32_t i = 0; i < std::thread::hardware_concurrency(); i++) {
    threads.push_back(new std::thread([&, i]() {
      int num = 100000000;
      while (num--) {
        collector.Collect(std::to_string(num % 10), num % 3);
      }
    }));
  }

  for (auto t : threads) {
    if (t && t->joinable()) {
      t->join();
    }
    delete t;
  }
}
