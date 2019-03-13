#include "../metrics_stash.h"
#include <random>
#include <unistd.h>

using namespace component;

using std::default_random_engine;


component::MetricsItem* new_histogram(int64_t val) {
  std::string name("test");
  auto item = new MetricsItem(MetricsType::kHistogram,
                              name,
                              val,
                              MetricsOption::kOptNone);
  return item;
}

int main(int argc, char** argv) {
  
  default_random_engine e;

  MetricsStash stash;
  stash.Start();

  uint32_t start = ::time(NULL);
  while(1) {
    if (::time(NULL) - start > 60) {
      break;
    }

    for (int i = 0; i < 10000; i++) {
      auto item = MetricsItemPtr(new_histogram(e() % 10000));
      stash.Stash(std::move(item));
    }
    ::usleep(10000);
  }

  ::sleep(30);
  return 0;
}
