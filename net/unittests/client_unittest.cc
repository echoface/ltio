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
#include "../tcp_channel.h"
#include "../socket_utils.h"
#include "../service_acceptor.h"
#include "../protocol/proto_service.h"
#include "../protocol/line/line_message.h"
#include "../protocol/http/http_request.h"
#include "../protocol/http/http_response.h"
#include "../protocol/proto_service_factory.h"
#include "../inet_address.h"
#include "clients/client_connector.h"
#include "clients/client_router.h"
#include "dispatcher/coro_dispatcher.h"
#include "base/coroutine/coroutine_runner.h"
#include "base/closure/closure_task.h"
#include "net/dispatcher/coro_dispatcher.h"
#include "net/protocol/raw/raw_message.h"
#include "net/protocol/raw/raw_proto_service.h"

TEST_CASE("client.base", "[http client]") {
  base::MessageLoop loop;
  loop.SetLoopName("client");
  loop.Start();

  net::InetAddress http_server_addr("127.0.0.1", 80);
  net::ClientRouter http_router(&loop, http_server_addr);
  net::CoroWlDispatcher* dispatcher_ = new net::CoroWlDispatcher(true);


  static const int connections = 10;

  net::RouterConf router_config;
  router_config.protocol = "http";
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
  }), 10000);

  loop.WaitLoopEnd();
}

TEST_CASE("client.http.request", "[http client send request]") {
  base::MessageLoop loop;
  loop.SetLoopName("client");
  loop.Start();

  net::InetAddress http_server_addr("127.0.0.1", 80);
  net::ClientRouter http_router(&loop, http_server_addr);
  net::CoroWlDispatcher* dispatcher_ = new net::CoroWlDispatcher(true);

  static const int connections = 10;

  net::RouterConf router_config;
  router_config.protocol = "http";
  router_config.recon_interval = 100;
  router_config.message_timeout = 5000;
  router_config.connections = connections;

  http_router.SetupRouter(router_config);
  http_router.SetWorkLoadTransfer(dispatcher_);

  http_router.StartRouter();

  int total_task = 10;
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

  sleep(2);

  for (int i = 0; i < total_task; i++) {
    co_go &loop << http_request_task;
  }

  loop.PostDelayTask(NewClosure([&](){
    REQUIRE(http_router.ClientCount() == connections);
    REQUIRE((failed_request + success_request) == total_task);

    http_router.StopRouterSync();
    loop.QuitLoop();
  }), 10000);

  loop.WaitLoopEnd();
}

TEST_CASE("client.raw.request", "[raw client send request]") {
	base::MessageLoop loop;
	loop.SetLoopName("client");
	loop.Start();

	net::InetAddress raw_server_addr("127.0.0.1", 5005);
	net::ClientRouter raw_router(&loop, raw_server_addr);
	net::CoroWlDispatcher* dispatcher_ = new net::CoroWlDispatcher(true);

	static const int connections = 10;

	net::RouterConf router_config;
	router_config.protocol = "raw";
	router_config.recon_interval = 100;
	router_config.message_timeout = 5000;
	router_config.connections = connections;

	raw_router.SetupRouter(router_config);
	raw_router.SetWorkLoadTransfer(dispatcher_);

	raw_router.StartRouter();

	int total_task = 10;
	int failed_request = 0;
	int success_request = 0;

	auto raw_request_task = [&]() {
		auto request = net::RawMessage::CreateRequest();
		request->SetMethod(12);
		request->SetContent("RawRequest");

		net::RawMessage* response = raw_router.SendRecieve(request);

		if (response && request->FailCode() == net::MessageCode::kSuccess) {
			success_request++;
		} else {
			LOG(INFO) << " failed reason:" << request->FailCode();
			failed_request++;
		}
	};

	sleep(2);

	for (int i = 0; i < total_task; i++) {
		co_go &loop << raw_request_task;
	}

	loop.PostDelayTask(NewClosure([&](){
		REQUIRE(raw_router.ClientCount() == connections);
		REQUIRE((failed_request + success_request) == total_task);

		raw_router.StopRouterSync();
		loop.QuitLoop();
	}), 10000);

	loop.WaitLoopEnd();
}
