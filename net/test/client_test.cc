#include <glog/logging.h>

#include <vector>
#include <thirdparty/catch/catch.hpp>
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
//#include "clients/client_channel.h"
#include "dispatcher/coro_dispatcher.h"
#include "base/coroutine/coroutine_runner.h"
#include "base/closure/closure_task.h"
#include "net/dispatcher/coro_dispatcher.h"
#include "net/protocol/raw/raw_message.h"
#include "net/protocol/raw/raw_proto_service.h"

base::MessageLoop loop;
base::MessageLoop wloop;
net::InetAddress server_address("127.0.0.1", 5006);
//net::InetAddress server_address("127.0.0.1", 80);
//net::InetAddress server_address("123.59.153.185", 80);
net::InetAddress raw_server_address("0.0.0.0", 5002);

net::ClientRouter*  router; //(base::MessageLoop*, const InetAddress&);
net::ClientRouter* raw_router;
net::RefHttpRequest g_request;
net::RefTcpChannel g_channel;

bool SendRequest(int sequence_id) {
  net::RefHttpRequest request = std::make_shared<net::HttpRequest>();
  request->SetKeepAlive(true);
  //request->SetRequestURL("/rtb?v=118");
  request->SetRequestURL("/");
  request->InsertHeader("Accept", "*/*");
  //request->InsertHeader("Host", "127.0.0.1");
  request->InsertHeader("Host", "g.test.amnetapi.com");
  request->InsertHeader("User-Agent", "curl/7.58.0");

  net::HttpResponse* response = router->SendRecieve(request);
  if (response && !response->IsKeepAlive()) {
    LOG(ERROR) << "recieve a closed response from server" << response->MessageDebug();
  }
  return response != NULL && request->MessageFailInfo() == net::FailInfo::kNothing;
}

void SendRawRequest(int sequence_id) {
  net::RefRawMessage raw_request = net::RawMessage::CreateRequest();
  std::string content("this is a raw request");
  raw_request->SetContent(content);
  raw_request->SetCode(12);
  raw_request->SetMethod(12);

  auto response = raw_router->SendRecieve(raw_request);
  if (response) {
    LOG(ERROR) << "Get RawResponse:\n" << response->MessageDebug();
  }
}

void HttpClientBenchMark(int grp, int count) {
  LOG(ERROR) << "@"<< base::CoroRunner::Runner().CurrentCoroutineId()
             << " group:" << grp << " start, count:" << count;
  int success_count = 0;
  for (int i=0; i < count; i++) {
    if (SendRequest(i)) {success_count++;}
  }
  LOG(ERROR) << "@"<< base::CoroRunner::Runner().CurrentCoroutineId()
             << " group:" << grp << " end, count/success:" << count << "/" << success_count;
}

int main(int argc, char* argv[]) {

  google::ParseCommandLineFlags(&argc, &argv, true);  // 初始化 gflags
  google::SetStderrLogging(google::GLOG_ERROR);

  net::CoroWlDispatcher* dispatcher_ = new net::CoroWlDispatcher(true);

  loop.SetLoopName("clientloop");
  wloop.SetLoopName("workloop");

  int groups = 2;
  int send_count_per_grp = 100;
  if (argc > 1) {
    groups = std::atoi(argv[1]);
  }
  if (argc > 2) {
    send_count_per_grp = std::atoi(argv[2]);
  }

  loop.Start();
  wloop.Start();

  router = new net::ClientRouter(&loop, server_address);
  net::RouterConf router_config;
  router_config.protocol = "http";
  router_config.connections = 10000;
  router_config.recon_interal = 1000;
  router_config.message_timeout = 5000;
  router->SetupRouter(router_config);
  router->SetWorkLoadTransfer(dispatcher_);
  loop.PostTask(NewClosure(std::bind(&net::ClientRouter::StartRouter, router)));

  sleep(2);

  for (int i = 0; i < groups; i++) {
    co_go &loop << std::bind(HttpClientBenchMark, i, send_count_per_grp);
  }

  loop.WaitLoopEnd();
  delete router;
  delete raw_router;
  return 0;
}
