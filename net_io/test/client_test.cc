#include <glog/logging.h>

#include <vector>
#include "inet_address.h"
#include "tcp_channel.h"
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
#include "base/coroutine/coroutine_runner.h"
#include "base/closure/closure_task.h"
#include "dispatcher/coro_dispatcher.h"
#include "protocol/raw/raw_message.h"
#include "protocol/raw/raw_proto_service.h"

base::MessageLoop loop;
base::MessageLoop wloop;

net::ClientRouter* http_router = NULL;
net::ClientRouter* raw_router = NULL;

bool SendRequest(int sequence_id) {
  net::RefHttpRequest request = std::make_shared<net::HttpRequest>();
  request->SetKeepAlive(true);
  request->SetRequestURL("/");
  request->InsertHeader("Accept", "*/*");
  request->InsertHeader("User-Agent", "curl/7.58.0");
  request->InsertHeader("T", std::to_string(sequence_id));

  net::HttpResponse* response = http_router->SendRecieve(request);
  if (response && !response->IsKeepAlive()) {
    LOG(ERROR) << "receive a closed response from server" << response->Dump();
  }
  if (request->FailCode() != net::MessageCode::kSuccess) {
    LOG(ERROR) << "request failed reason:" << request->FailCode();
  }
  return response != NULL && request->FailCode() == net::MessageCode::kSuccess;
}

void SendRawRequest(int sequence_id) {
  auto raw_request = net::LtRawMessage::Create(true);
  std::string content("this is a raw request");
  raw_request->SetContent(content);
  auto header = raw_request->MutableHeader();
  header->method = 12;

  auto response = raw_router->SendRecieve(raw_request);
  if (response) {
    LOG(ERROR) << "Get RawResponse:\n" << response->Dump();
  }
}
int g_count = 0;

void HttpClientBenchMark(int grp, int count) {
  LOG(ERROR) << "@"<< base::CoroRunner::Runner().CurrentCoroutineId()
             << " group:" << grp << " start, count:" << count;

  int success_count = 0;
  for (int i=0; i < count; i++) {
    co_sleep(100);
    if (SendRequest(g_count++)) {success_count++;}
  }

  LOG(ERROR) << "@"<< base::CoroRunner::Runner().CurrentCoroutineId()
             << " group:" << grp << " end, count/success:" << count << "/" << success_count;
}

// usage
int main(int argc, char* argv[]) {
  //gflags::ParseCommandLineFlags(&argc, &argv, true);  // 初始化 gflags
  google::SetStderrLogging(google::GLOG_ERROR);
  //google::ParseCommandLineFlags(&argc, &argv, true);

  loop.SetLoopName("clientloop");
  wloop.SetLoopName("workloop");

  loop.Start();
  wloop.Start();
  {
    net::url::SchemeIpPort server_info;
    LOG_IF(ERROR, !net::url::ParseURI("http://127.0.0.1:80", server_info)) << " server can't be resolve";
    http_router = new net::ClientRouter(&loop, server_info);

    net::RouterConf router_config;
    router_config.connections = 1;
    router_config.recon_interval = 100;
    router_config.message_timeout = 5000;
    http_router->SetupRouter(router_config);

    http_router->StartRouter();
  }

  sleep(2);

  for (size_t i = 0; i < std::thread::hardware_concurrency(); i++) {
    co_go &loop << std::bind(HttpClientBenchMark, i, 100);
  }

  loop.WaitLoopEnd();

  delete http_router;
  delete raw_router;
  return 0;
}
