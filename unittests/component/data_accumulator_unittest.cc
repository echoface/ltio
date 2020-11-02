#include <unistd.h>
#include <iostream>
#include <unistd.h>
#include <stdlib.h>
#include <atomic>
#include <thread>
#include "glog/logging.h"
#include <base/time/time_utils.h>
#include <components/utils/data_accumulator.h>

#include <catch/catch.hpp>

TEST_CASE("cm_accumulator", "[acc data]") {
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
      while(num--) {
        collector.Collect(std::to_string(num%10), num%3);
        usleep(50);
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
