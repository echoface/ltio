#include <memory>
#include <atomic>
#include <functional>
#include <glog/logging.h>
#include <gflags/gflags.h>
#include "clients/client.h"
#include "coroutine/coroutine_runner.h"
#include "protocol/raw/raw_proto_service.h"
#include "protocol/proto_service_factory.h"
#include "fw_rapid_message.h"

std::string arg_host;

using namespace lt;

typedef std::unique_ptr<net::Client> ClientPtr;
ClientPtr raw_router;

void RequestRepeatedTask(std::string content) {
  auto raw_request = FwRapidMessage::Create(true);
  auto header = raw_request->Header();
  header->cmdid = 1000;
  raw_request->SetContent(content);
  FwRapidMessage* response = raw_router->SendRecieve(raw_request);
  if (response) {
    std::cout << "res:" << response->Content()  << std::endl;
  } else {
    std::cout << "err:" << raw_request->FailCode() << std::endl;
  }
}

int main(int argc, char** argv) {
  gflags::ParseCommandLineFlags(&argc, &argv, true);
  base::MessageLoop mainloop;

  net::ProtoServiceFactory::Instance().RegisterCreator("rapid", []() -> net::RefProtoService {
    auto service = std::make_shared<net::RawProtoService<FwRapidMessage>>();
    return std::static_pointer_cast<net::ProtoService>(service);
  });

  net::ClientConfig config;
  net::url::RemoteInfo server_info;

  if (argc < 2) {
    std::cout << "format error, eg:./raw_client ip:port" << std::endl;
    return -1;
  }
  arg_host = argv[1];
  if (!net::url::ParseRemote("rapid://" + arg_host, server_info)) {
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
  while (1) {
    std::string content;
    std::cout << "send:";
    std::flush(std::cout);
    std::getline(std::cin, content);
    if (content == "quit") {
      raw_router->FinalizeSync();
      mainloop.QuitLoop();
      break;
    }
    co_go &mainloop << std::bind(RequestRepeatedTask, content);
    std::this_thread::sleep_for(std::chrono::seconds(1));
  };
  mainloop.WaitLoopEnd();
}
