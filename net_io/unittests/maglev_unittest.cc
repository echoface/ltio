#include <iostream>
#include <unistd.h>
#include <stdlib.h>
#include <atomic>

#include "glog/logging.h"
#include <time/time_utils.h>
#include <coroutine/coroutine.h>
#include <message_loop/message_loop.h>
#include <coroutine/coroutine_runner.h>

#include <catch/catch.hpp>

#include "../clients/router/maglev_router.h"

using Endpoint = net::MaglevHelper::Endpoint;

TEST_CASE("client.maglev", "[meglev]") {
  LOG(INFO) << " start test client.maglev";

  std::vector<Endpoint> eps;
  for (int i = 0; i < 10; i++) {
    Endpoint s = {i, 1, 10000*(i+1) + (100 * i)}; 
    LOG(INFO) << s.num << ", " << s.weight << ", " << s.hash;
    eps.push_back(s);
  }

  LOG(INFO) << " GenerateMaglevHash";
  auto lookup_table = net::MaglevHelper::GenerateMaglevHash(eps);
  LOG(INFO) << " GenerateMaglevHash success";

  std::map<uint32_t, uint32_t> freq;
  for (auto v : lookup_table) {
    freq[v]++;
  }

  for (auto kv : freq) {
    double p = double(kv.second) / net::kDefaultChRingSize;
    LOG(INFO) << "ep:" << kv.first << " p:" << std::round(100*p) << "%"; 
  }

  LOG(INFO) << " end test maglev end";
}
