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

void RequestRepeatedTask(const std::string& content) {
  auto raw_request = net::LtRawMessage::Create(true);
  raw_request->SetMethod(12);
  raw_request->SetContent(content);
  net::LtRawMessage* response = raw_router->SendRecieve(raw_request);
  if (response) {
    std::cout << "res:" << response->Dump() << std::endl;
  } else {
    std::cout << "err:" << raw_request->FailCode() << std::endl;
  }
}

int main(int argc, char** argv) {
  gflags::ParseCommandLineFlags(&argc, &argv, true);
  base::MessageLoop mainloop;

  net::ClientConfig config;
  net::url::RemoteInfo server_info;

  if (argc < 2) {
    std::cout << "format error, eg:./raw_client ip:port" << std::endl;
    return -1;
  }
  arg_host = argv[1];
  if (!net::url::ParseRemote("raw://" + arg_host, server_info)) {
    LOG(ERROR) << " address" << arg_host << " not correct";
    return -1;
  }

  mainloop.SetLoopName("main");
  mainloop.Start();

  raw_router.reset(new net::Client(&mainloop, server_info));

  config.connections = 4;
  config.recon_interval = 1000;
  config.message_timeout = 5000;

  raw_router->Initialize(config);
  std::this_thread::sleep_for(std::chrono::seconds(1));
  while(1) {
    std::string line;
    std::cout << "send:";
    std::flush(std::cout);
    std::getline(std::cin, line);
    if (line == "quit") {
      raw_router->FinalizeSync();
      mainloop.QuitLoop();
      break;
    }
    co_go &mainloop << std::bind(RequestRepeatedTask, line);
    std::this_thread::sleep_for(std::chrono::seconds(1));
  }
  mainloop.WaitLoopEnd();
}
