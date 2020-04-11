//
// Created by gh on 18-12-23.
//

#include <unistd.h>
#include <iostream>
#include <unistd.h>
#include <stdlib.h>
#include <atomic>
#include "glog/logging.h"
#include <base/time/time_utils.h>
#include "base/closure/closure_task.h"
#include <base/coroutine/coroutine.h>
#include <base/message_loop/message_loop.h>
#include <base/coroutine/coroutine_runner.h>
#include "net_io/tcp_channel.h"
#include "net_io/socket_utils.h"
#include "net_io/address.h"
#include "net_io/url_utils.h"
#include "net_io/socket_acceptor.h"
#include "net_io/codec/codec_service.h"
#include "net_io/codec/line/line_message.h"
#include "net_io/codec/http/http_request.h"
#include "net_io/codec/http/http_response.h"
#include "net_io/codec/codec_factory.h"
#include "net_io/dispatcher/coro_dispatcher.h"
#include "net_io/dispatcher/coro_dispatcher.h"
#include "net_io/codec/raw/raw_message.h"
#include "net_io/codec/raw/raw_codec_service.h"
#include "net_io/clients/client.h"
#include "net_io/clients/client_connector.h"

#include <thirdparty/catch/catch.hpp>

using namespace lt;

TEST_CASE("url.host_resolve", "[host resolve test]") {
  std::string host_ip;
  net::url::HostResolve("g.test.amnetapi.com", host_ip);
  std::cout << "result:" << host_ip << std::endl;
}

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

TEST_CASE("uri.parse.remote", "[remote uri parse]") {
  std::vector<std::string> remote_uris = {
    "http://gh:passwd@localhost:8020?abc=&name=1234",
    "://gh:passwd@localhost:8020?abc=&name=1234",
    "gh:passwd@localhost:8020?abc=&name=1234",
    "gh:@localhost:8020?abc=&name=1234",
    "gh@localhost:8020?abc=&name=1234",
    "gh@localhost?abc=&name=1234",
    "gh@localhost?",
    "gh@localhost",
    "localhost:80?abc=",
  };

  for (auto remote_uri : remote_uris) {
    net::url::RemoteInfo remote;
    remote.reset();

    net::url::ParseRemote(remote_uri, remote, true);
    LOG(INFO) << ">>>>>>:" << remote_uri;
    LOG(INFO) << "protocol:" << remote.protocol
      << " user:" << remote.user
      << " psd:" << remote.passwd
      << " host:" << remote.host
      << " ip:" << remote.host_ip
      << " port:" << remote.port
      << " query:" << remote.querys.size();
  }
}

