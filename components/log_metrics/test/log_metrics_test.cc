#include "../metrics_stash.h"
#include <random>
#include <unistd.h>
#include <thread>
#include <functional>
#include <iostream>

using namespace component;

MetricsStash stash;

using std::default_random_engine;
default_random_engine e;
int running = 1;


component::MetricsItem* new_histogram(const std::string& topic, int64_t val) {
  auto item = new MetricsItem(MetricsType::kHistogram, topic, val,
                              MetricsOption::kOptNone);
  return item;
}

void log_main(std::string log_topic) {
  while(running) {
    for (int i = 0; i < 10000; i++) {
      auto item = MetricsItemPtr(new_histogram(log_topic, 100 + e() % 500));
      stash.Stash(std::move(item));
    }
    ::usleep(1000);
  }
}

int main(int argc, char** argv) {
  stash.RegistDistArgs("http_requests",
                       {.dist_precise = 1,.dist_range_min = 50,.dist_range_max = 1000});
  stash.EnableTurboMode(true);
  stash.Start();

  std::thread t1(std::bind(log_main, "failed_count"));
  std::thread t2(std::bind(log_main, "http_requests"));

  while(running) {
    std::cin >> running;
  }

  if (t1.joinable()) {
    t1.join();
  }
  if (t2.joinable()) {
    t2.join();
  }

  ::sleep(5);
  stash.StopSync();
  return 0;
}
