#include <memory>
#include <atomic>
#include <glog/logging.h>
#include <gflags/gflags.h>
#include "clients/client_router.h"
#include "coroutine/coroutine_runner.h"
#include "protocol/raw/raw_message.h"
#include "protocol/raw/raw_proto_service.h"

int FLAGS_t = 1;
int FLAGS_c = 5000;
int FLAGS_n = 1000000;
std::string FLAGS_h;
#if 0
DEFINE_int32(t, 1, "thread count");
DEFINE_int32(c, 100, "coroutine concurrency count");
DEFINE_int32(n, 100000, "request count");
DEFINE_string(h, "", "raw server address,[host:port]");
#endif

typedef std::unique_ptr<net::ClientRouter> ClientRouterPtr;

ClientRouterPtr raw_router;

std::atomic_uint64_t request_counter;
std::atomic_uint64_t failed_counter;

std::vector<base::MessageLoop*> loops;

void InitThreadLoops(int count) {
  for (int i = 0; i < count; i++) {
    auto loop = new base::MessageLoop();
    loop->SetLoopName("loop_" + std::to_string(i));
    loop->Start();
    loops.push_back(loop);
  }
}

void ContentMain() {
  LOG(INFO) << __FUNCTION__ << " enter";
  while(request_counter++ < FLAGS_n) {
    auto raw_request = net::LtRawMessage::Create(true);
    std::string content("request content");
    raw_request->SetContent(content);
    auto header = raw_request->MutableHeader();
    header->method = 12;

    auto response = raw_router->SendRecieve(raw_request);

    if (response) {
      LOG_EVERY_N(INFO, 1000) << " response:" << response->Dump();
    } else {
      failed_counter++;
    }
  }
  LOG(INFO) << " bench finished, total:" <<  FLAGS_n
    << " failed:" << failed_counter
    << " success:" << FLAGS_n - failed_counter;
}

int main(int argc, char** argv) {
  //gflags::ParseCommandLineFlags(&argc, &argv, true);
  //google::ParseCommandLineFlags(&argc, &argv, true);
  base::MessageLoop mainloop;

  failed_counter.store(0);
  request_counter.store(0);

  net::RouterConf router_config;
  net::url::SchemeIpPort server_info;

  if (argc < 2) {
    LOG(ERROR) << " address not provided";
    return -1;
  }
  FLAGS_h = argv[1];
  if (!net::url::ParseURI("raw://" + FLAGS_h, server_info)) {
    LOG(ERROR) << " address" << FLAGS_h << " not correct";
    return -1;
  }

  if (FLAGS_c < 1 || FLAGS_c > 10000) {
    LOG(ERROR) << " concurrency number [" << FLAGS_c << "] out of range";
    return -1;
  }
  if (FLAGS_t < 1 || FLAGS_t > std::thread::hardware_concurrency()) {
    LOG(ERROR) << " thread number [" << FLAGS_c << "] out of range";
    return -1;
  }

  InitThreadLoops(FLAGS_t);

  mainloop.SetLoopName("main");
  mainloop.Start();

  raw_router.reset(new net::ClientRouter(&mainloop, server_info));

  router_config.connections = FLAGS_t * 4;
  router_config.recon_interval = 100;
  router_config.message_timeout = 500;

  raw_router->SetupRouter(router_config);
  raw_router->StartRouter();
  LOG(INFO) << " start request raw server";
  for (auto loop : loops) {
    for (int i = 0; i < FLAGS_c; i++) {
      co_go loop << std::bind(ContentMain);
    }
  }
  mainloop.WaitLoopEnd();
}


