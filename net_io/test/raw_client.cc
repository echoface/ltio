#include <memory>
#include <atomic>
#include <functional>
#include <glog/logging.h>
#include <gflags/gflags.h>
#include "clients/client.h"
#include "coroutine/coroutine_runner.h"
#include "protocol/raw/raw_message.h"
#include "protocol/raw/raw_proto_service.h"

std::string arg_host;

using namespace lt;

typedef std::unique_ptr<net::Client> ClientPtr;
ClientPtr raw_router;

void SendRawRequest(net::LtRawMessage::RefRawMessage& message) {
  net::LtRawMessage* response = raw_router->SendRecieve(message);
  if (response) {
    std::cout << "res:" << response->Dump() << std::endl;
  } else {
    std::cout << "err:" << message->FailCode() << std::endl;
  }
}

int main(int argc, char** argv) {
  //gflags::ParseCommandLineFlags(&argc, &argv, true);
  //google::ParseCommandLineFlags(&argc, &argv, true);
  base::MessageLoop mainloop;

  net::ClientConfig config;
  net::url::SchemeIpPort server_info;

  if (argc < 2) {
    std::cout << "format error, eg:./raw_client ip:port" << std::endl;
    return -1;
  }
  arg_host = argv[1];
  if (!net::url::ParseURI("raw://" + arg_host, server_info)) {
    LOG(ERROR) << " address" << arg_host << " not correct";
    return -1;
  }

  mainloop.SetLoopName("main");
  mainloop.Start();

  raw_router.reset(new net::Client(&mainloop, server_info));

  config.connections = 5;
  config.recon_interval = 100;
  config.message_timeout = 500;

  raw_router->Initialize(config);

  while (1) {
    uint32_t method = 0;
    std::string content;
    std::cout << "input method:";
    std::cin >> method;
    std::cout << "intpu content:";
    std::cin >> content;

    auto raw_request = net::LtRawMessage::Create(true);
    auto header = raw_request->MutableHeader();

    header->method = method;
    raw_request->SetContent(content);

    co_go &mainloop << std::bind(SendRawRequest, raw_request);
  }
  mainloop.WaitLoopEnd();
}
