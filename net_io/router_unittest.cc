#include <base/coroutine/co_runner.h>
#include <base/message_loop/message_loop.h>
#include <base/time/time_utils.h>
#include <stdlib.h>
#include <unistd.h>
#include <atomic>
#include <iostream>
#include "glog/logging.h"
#include "hash/murmurhash3.h"

#include <net_io/clients/router/client_router.h>
#include <net_io/clients/router/hash_router.h>
#include <net_io/clients/router/ringhash_router.h>
#include <net_io/clients/router/roundrobin_router.h>

#include <catch2/catch_test_macros.hpp>

struct Hasher {
  uint64_t operator()(const std::string& key) {
    uint32_t out = 0;
    MurmurHash3_x86_32(key.data(), key.size(), 0x80000000, &out);
    return out;
  }
};

struct RouterManager : public lt::net::ClientDelegate {
  base::MessageLoop* NextIOLoopForClient() { return NULL; }
};

extern RouterManager router_delegate;

TEST_CASE("client.hashrouter", "[http client]") {
  LOG(INFO) << " start hashrouter test enter";

  base::MessageLoop loop;
  loop.SetLoopName("client");
  loop.Start();

  std::vector<std::string> remote_hosts = {
      "redis://127.0.0.1:6379", "redis://127.0.0.1:6379",
      "redis://127.0.0.1:6379", "redis://127.0.0.1:6379"};

  lt::net::ClientConfig config;

  config.recon_interval = 10;
  config.message_timeout = 5000;
  config.connections = 10;

  {
    lt::net::HashRouter<Hasher> router;
    for (auto& remote : remote_hosts) {
      lt::net::url::RemoteInfo server_info;
      bool success = lt::net::url::ParseRemote(remote, server_info);
      LOG_IF(ERROR, !success) << " server:" << remote << " can't be resolve";
      if (!success) {
        LOG(INFO) << "host:" << server_info.host << " ip:" << server_info.host_ip
          << " port:" << server_info.port
          << " protocol:" << server_info.scheme;
        return;
      }

      lt::net::RefClient client(new lt::net::Client(&loop, server_info));
      client->SetDelegate(&router_delegate);
      client->Initialize(config);
      router.AddClient(std::move(client));
    }

    std::map<intptr_t, int> use_couter;
    for (int i = 0; i < 1000; i++) {
      std::string key = std::to_string(i);
      lt::net::RefClient c = router.GetNextClient(key, NULL);
      if (c) {
        use_couter[(intptr_t(c.get()))]++;
      }
      LOG_IF(INFO, c != NULL) << "this@" << c << ", detail:" << c->ClientInfo();
    }
    for (auto kv : use_couter) {
      LOG(INFO) << "this@" << (void*)kv.first << ", cout:" << kv.second;
    }
  }

  loop.PostDelayTask(NewClosure([&]{
    loop.QuitLoop();
  }), 1000);
  loop.WaitLoopEnd();
  LOG(INFO) << " end test client.base, http client connections";
}
