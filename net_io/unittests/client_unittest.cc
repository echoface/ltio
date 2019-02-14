#include <unistd.h>
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

#include <glog/logging.h>
#include "tcp_channel.h"
#include "inet_address.h"
#include "socket_utils.h"
#include "service_acceptor.h"
#include "protocol/proto_service.h"
#include "protocol/line/line_message.h"
#include "protocol/http/http_request.h"
#include "protocol/http/http_response.h"
#include "protocol/proto_service_factory.h"
#include "clients/client_connector.h"
#include "clients/client_router.h"
#include "dispatcher/coro_dispatcher.h"
#include "base/closure/closure_task.h"
#include "dispatcher/coro_dispatcher.h"
#include "protocol/raw/raw_message.h"
#include "protocol/raw/raw_proto_service.h"

static std::atomic_int io_round_count;

const int bench_count = 1000000;
const int bench_concurrent = 50;

std::vector<base::MessageLoop*> InitLoop() {
  std::vector<base::MessageLoop*> lops;
  for (uint32_t i = 0; i < std::thread::hardware_concurrency(); i++) {
    base::MessageLoop* l = new base::MessageLoop();
    l->SetLoopName("io_" + std::to_string(i));
    l->Start();
    lops.push_back(l);
  }
  return lops;
}

std::vector<base::MessageLoop*> loops;

class RouterManager: public net::RouterDelegate {
  public:
    base::MessageLoop* NextIOLoopForClient() {
      if (loops.empty()) {
        return NULL;
      }
      io_round_count++;
      return loops[io_round_count % loops.size()];
    }
};
RouterManager router_delegate;

TEST_CASE("client.base", "[http client]") {
  LOG(INFO) << " start test client.base, http client connections";

  base::MessageLoop loop;
  loop.SetLoopName("client");
  loop.Start();
  static const int connections = 10;

  net::url::SchemeIpPort server_info;
  LOG_IF(ERROR, !net::url::ParseURI("http://127.0.0.1:80", server_info)) << " server can't be resolve";

  net::ClientRouter http_router(&loop, server_info);
  net::CoroWlDispatcher* dispatcher_ = new net::CoroWlDispatcher(true);


  net::RouterConf router_config;
  router_config.recon_interval = 100;
  router_config.message_timeout = 5000;
  router_config.connections = connections;

  http_router.SetupRouter(router_config);
  http_router.SetWorkLoadTransfer(dispatcher_);

  http_router.StartRouter();

  loop.PostDelayTask(NewClosure([&](){
	  REQUIRE(http_router.ClientCount() == connections);
	  http_router.StopRouterSync();
    loop.QuitLoop();
  }), 500);

  loop.WaitLoopEnd();

  LOG(INFO) << " end test client.base, http client connections";
}

TEST_CASE("client.http.request", "[http client send request]") {
  LOG(INFO) << " start test client.http.request, http client send request";

  base::MessageLoop loop;
  loop.SetLoopName("client");
  loop.Start();

  static const int connections = 10;
  net::CoroWlDispatcher* dispatcher_ = new net::CoroWlDispatcher(true);

  net::url::SchemeIpPort server_info;
  LOG_IF(ERROR, !net::url::ParseURI("http://127.0.0.1:80", server_info)) << " server can't be resolve";

  net::ClientRouter http_router(&loop, server_info);

  net::RouterConf router_config;
  router_config.recon_interval = 100;
  router_config.message_timeout = 5000;
  router_config.connections = connections;

  http_router.SetupRouter(router_config);
  http_router.SetWorkLoadTransfer(dispatcher_);

  http_router.StartRouter();

  int total_task = 1;
  int failed_request = 0;
  int success_request = 0;

  auto http_request_task = [&]() {
    net::RefHttpRequest request = std::make_shared<net::HttpRequest>();
    request->SetKeepAlive(true);
    request->SetRequestURL("/");
    request->InsertHeader("Accept", "*/*");
    request->InsertHeader("Host", "127.0.0.1");
    request->InsertHeader("User-Agent", "curl/7.58.0");

    net::HttpResponse* response = http_router.SendRecieve(request);
    if (response && request->FailCode() == net::MessageCode::kSuccess) {
    	success_request++;
    } else {
      failed_request++;
    }
  };

  sleep(1);

  for (int i = 0; i < total_task; i++) {
    co_go &loop << http_request_task;
  }

  loop.PostDelayTask(NewClosure([&](){
    REQUIRE(http_router.ClientCount() == connections);
    REQUIRE((failed_request + success_request) == total_task);

    http_router.StopRouterSync();
    loop.QuitLoop();
  }), 5000);

  loop.WaitLoopEnd();

  LOG(INFO) << " end test client.http.request, http client send request";
}

TEST_CASE("client.raw.request", "[raw client send request]") {
  LOG(INFO) << ">>>>>>> start test client.raw.request, raw client send request";

  base::MessageLoop loop;
  loop.SetLoopName("client");
  loop.Start();

  net::url::SchemeIpPort server_info;
  LOG_IF(ERROR, !net::url::ParseURI("raw://127.0.0.1:5005", server_info)) << " server can't be resolve";

  net::ClientRouter raw_router(&loop, server_info);
  net::CoroWlDispatcher* dispatcher_ = new net::CoroWlDispatcher(true);

  static const int connections = 10;

  net::RouterConf router_config;
  router_config.recon_interval = 1000;
  router_config.message_timeout = 5000;
  router_config.connections = connections;

  raw_router.SetupRouter(router_config);
  raw_router.SetWorkLoadTransfer(dispatcher_);

  raw_router.StartRouter();

  int total_task = 1;
  int failed_request = 0;
  int success_request = 0;

  auto raw_request_task = [&]() {

    auto request = net::LtRawMessage::Create(true);
    auto lt_header = request->MutableHeader();
    lt_header->code = 0;
    lt_header->method = 12;
    request->SetContent("RawRequest");

    net::LtRawMessage* response = raw_router.SendRecieve(request);

    if (response && request->FailCode() == net::MessageCode::kSuccess) {
      success_request++;
    } else {
      LOG(INFO) << " failed reason:" << request->FailCode();
      failed_request++;
    }
  };

  sleep(1);

  for (int i = 0; i < total_task; i++) {
    co_go &loop << raw_request_task;
  }

  loop.PostDelayTask(NewClosure([&](){
    REQUIRE(raw_router.ClientCount() == connections);
    REQUIRE((failed_request + success_request) == total_task);

    raw_router.StopRouterSync();
    loop.QuitLoop();
  }), 5000);

  loop.WaitLoopEnd();
  LOG(INFO) << "<<<<<< end test client.raw.request, raw client send request";
}

TEST_CASE("client.timer.request", "[fetch resource every interval]") {
  LOG(INFO) << " start test client.timer.request, raw client send request";

  base::MessageLoop loop;
  loop.SetLoopName("client");
  loop.Start();

  net::url::SchemeIpPort server_info;
  LOG_IF(ERROR, !net::url::ParseURI("raw://127.0.0.1:5005", server_info)) << " server can't be resolve";

  net::ClientRouter raw_router(&loop, server_info);
  net::CoroWlDispatcher* dispatcher_ = new net::CoroWlDispatcher(true);

  static const int connections = 10;

  {
    net::RouterConf router_config;
    router_config.heart_beat_ms = 5000;
    router_config.recon_interval = 1000;
    router_config.message_timeout = 5000;
    router_config.connections = connections;

    raw_router.SetupRouter(router_config);
    raw_router.SetWorkLoadTransfer(dispatcher_);

    raw_router.StartRouter();
  }

  int send_count = 10;

  co_go &loop << [&]() {
    do {
      auto request = net::LtRawMessage::Create(true);
      auto lt_header = request->MutableHeader();
      lt_header->code = 0;
      lt_header->method = 12;
      request->SetContent("RawRequest");

      net::LtRawMessage* response = raw_router.SendRecieve(request);
      if (response && request->FailCode() == net::MessageCode::kSuccess) {
        // success  do your things
      }
      co_sleep(50);
    } while(send_count--);

    loop.PostTask(NewClosure([&](){

      raw_router.StopRouterSync();

      loop.PostTask(NewClosure([&](){
        loop.QuitLoop();
      }));

    }));
  };

  loop.PostDelayTask(NewClosure([&](){
    LOG(INFO) << " timer.request ut timeout";
    loop.QuitLoop();
  }), 10000);

  loop.WaitLoopEnd();
  LOG(INFO) << "system co count:" << base::SystemCoroutineCount();

  LOG(INFO) << " end test client.timer.request, raw client send request";
}


TEST_CASE("client.raw.bench", "[raw client send request benchmark]") {

  LOG(INFO) << " start test client.raw.bench, raw client send request benchmark";
  base::MessageLoop loop;
  loop.SetLoopName("client");
  loop.Start();

  if (loops.empty()) {
    loops = InitLoop();
  }

  static std::atomic_int io_round_count;
  class RouterManager: public net::RouterDelegate {
    public:
      base::MessageLoop* NextIOLoopForClient() {
        if (loops.empty()) {
          return NULL;
        }
        io_round_count++;
        return loops[io_round_count % loops.size()];
      }
  };
  RouterManager router_delegate;

  net::url::SchemeIpPort server_info;
  LOG_IF(ERROR, !net::url::ParseURI("raw://127.0.0.1:5005", server_info)) << " server can't be resolve";

  net::ClientRouter raw_router(&loop, server_info);
  net::CoroWlDispatcher* dispatcher_ = new net::CoroWlDispatcher(true);
  dispatcher_->SetWorkerLoops(loops);

  static const int connections = 10;

  net::RouterConf router_config;
  router_config.recon_interval = 10;
  router_config.message_timeout = 5000;
  router_config.connections = connections;

  raw_router.SetupRouter(router_config);
  raw_router.SetWorkLoadTransfer(dispatcher_);
  raw_router.SetDelegate(&router_delegate);
  raw_router.StartRouter();

  std::atomic_int64_t  total_task;
  total_task = bench_count;

  std::atomic_int64_t failed_request;
  failed_request = 0;
  std::atomic_int64_t success_request;
  success_request = 0;

  sleep(5);

  auto raw_request_task = [&]() {
    while(total_task-- > 0) {

      auto request = net::LtRawMessage::Create(true);
      auto lt_header = request->MutableHeader();
      lt_header->code = 0;
      lt_header->method = 12;
      request->SetContent("RawRequest");

      net::LtRawMessage* response = raw_router.SendRecieve(request);

      if (response && request->FailCode() == net::MessageCode::kSuccess) {
        success_request++;
      } else {
        failed_request++;
      }

      if (failed_request + success_request == bench_count) {
        LOG(INFO) << " start bench finished.............<<<<<<"
          << "success:" << success_request
          << " failed: " << failed_request
          << " test_count:" << bench_count;
        raw_router.StopRouterSync();
        loop.QuitLoop();
      }
    };
  };

  LOG(INFO) << " start bench started.............<<<<<<";
  for (int i = 0; i < bench_concurrent; i++) {
    auto l = loops[i % loops.size()];
    co_go l << raw_request_task;
  }

  loop.WaitLoopEnd();
}

TEST_CASE("client.http.bench", "[http client send request benchmark]") {
  LOG(INFO) << " start test client.http.bench, http client send request benchmark";

  base::MessageLoop loop;
  loop.SetLoopName("client");
  loop.Start();

  if (loops.empty()) {
    loops = InitLoop();
  }

  net::url::SchemeIpPort server_info;
  //LOG_IF(ERROR, !net::url::ParseURI("http://www.ltio.com:5006", server_info)) << " server can't be resolve";
  LOG_IF(ERROR, !net::url::ParseURI("http://127.0.0.1:5006", server_info)) << " server can't be resolve";

  LOG(INFO) << "parse result, host:" << server_info.host
    << " ip:" << server_info.host_ip
    << " port:" << server_info.port
    << " protocol:" << server_info.protocol;

  net::ClientRouter http_router(&loop, server_info);
  net::CoroWlDispatcher* dispatcher_ = new net::CoroWlDispatcher(true);
  dispatcher_->SetWorkerLoops(loops);

  const int connections = 10;

  net::RouterConf router_config;
  router_config.recon_interval = 10;
  router_config.message_timeout = 5000;
  router_config.connections = connections;

  http_router.SetupRouter(router_config);
  http_router.SetWorkLoadTransfer(dispatcher_);
  http_router.SetDelegate(&router_delegate);
  http_router.StartRouter();

  std::atomic_int64_t  total_task;
  total_task = bench_count;

  std::atomic_int64_t failed_request;
  failed_request = 0;
  std::atomic_int64_t success_request;
  success_request = 0;

  sleep(5);

  auto request_task = [&]() {

    while(total_task-- > 0) {

      net::RefHttpRequest request = std::make_shared<net::HttpRequest>();
      request->SetKeepAlive(true);
      request->SetRequestURL("/");
      request->InsertHeader("Accept", "*/*");
      request->InsertHeader("User-Agent", "curl/7.58.0");

      net::HttpResponse* response = http_router.SendRecieve(request);

      if (response && request->FailCode() == net::MessageCode::kSuccess) {
        success_request++;
      } else {
        failed_request++;
      }

      if (failed_request + success_request == bench_count) {
        LOG(INFO) << " start bench finished.............<<<<<<"
          << "success:" << success_request
          << " failed: " << failed_request
          << " test_count:" << bench_count;
        http_router.StopRouterSync();
        loop.QuitLoop();
      }
    };
  };

  LOG(INFO) << " start bench started.............<<<<<<";
  for (int i = 0; i < bench_concurrent; i++) {
    auto l = loops[i % loops.size()];
    l->PostTask(NewClosure([=]() {
      co_go request_task;
    }));
  }
  loop.WaitLoopEnd();
}
