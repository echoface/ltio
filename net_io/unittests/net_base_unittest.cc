//
// Created by gh on 18-12-23.
//

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
#include "socket_utils.h"
#include "inet_address.h"
#include "service_acceptor.h"
#include "url_string_utils.h"
#include "protocol/proto_service.h"
#include "protocol/line/line_message.h"
#include "protocol/http/http_request.h"
#include "protocol/http/http_response.h"
#include "protocol/proto_service_factory.h"
#include "clients/client_connector.h"
#include "clients/client.h"
#include "dispatcher/coro_dispatcher.h"
#include "base/closure/closure_task.h"
#include "dispatcher/coro_dispatcher.h"
#include "protocol/raw/raw_message.h"
#include "protocol/raw/raw_proto_service.h"

TEST_CASE("uri.parse", "[http uri parse]") {
  {
    net::url::SchemeIpPort result;
    REQUIRE(net::url::ParseURI("http://www.baidu.com:80", result));
    REQUIRE(result.port == 80);
    REQUIRE(result.protocol == "http");
    REQUIRE(result.host == "www.baidu.com");
  }

  {
    net::url::SchemeIpPort result;
    REQUIRE(net::url::ParseURI("://127.0.0.1:", result));
    REQUIRE(result.protocol == "http");
    REQUIRE(result.host == "127.0.0.1");
  }

  {
    net::url::SchemeIpPort result;
    REQUIRE(net::url::ParseURI("://61.135.169.121:5006", result));
    REQUIRE(result.protocol == "http");
    REQUIRE(result.host == "61.135.169.121");
    LOG(INFO) << " result host_ip:" << result.host_ip;
  }

}


