#include <memory>
#include <atomic>
#include <functional>
#include <glog/logging.h>
#include <gflags/gflags.h>
#include "net_io/clients/client.h"
#include "base/coroutine/coroutine_runner.h"
#include "net_io/protocol/raw/raw_message.h"
#include "net_io/protocol/raw/raw_proto_service.h"

#include "fw_rapid_message.h"

using namespace lt;

typedef std::unique_ptr<net::Client> ClientPtr;
ClientPtr raw_router;

void DoLtRawRequest(const std::string& content) {
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

void DoFwRapidRequest(std::string content) {
  auto raw_request = FwRapidMessage::Create(true);
  raw_request->SetCmdId(100);
  raw_request->SetContent(content);
  FwRapidMessage* response = raw_router->SendRecieve(raw_request);
  if (response) {
    std::cout << "res:" << response->Content()  << std::endl;
  } else {
    std::cout << "err:" << raw_request->FailCode() << std::endl;
  }
}


DEFINE_string(remote, "", "scheme://host:port remote address");

void CodecInitialize() {
  net::ProtoServiceFactory::RegisterCreator(
    "rapid", [](base::MessageLoop* loop) -> net::RefProtoService {
      auto service = std::make_shared<net::RawProtoService<FwRapidMessage>>(loop);
      return std::static_pointer_cast<net::ProtoService>(service);
    });
}

int main(int argc, char** argv) {
  gflags::ParseCommandLineFlags(&argc, &argv, true);
  gflags::SetUsageMessage("usage: ./lt_client_cli --remote=scheme://host:port[?[supported queries]]");

  if (FLAGS_remote.empty()) {
    gflags::ShowUsageWithFlags(argv[0]);
    return -1;
  }

  CodecInitialize();

  base::MessageLoop mainloop;

  net::ClientConfig config;
  net::url::RemoteInfo server_info;

  if (!net::url::ParseRemote(FLAGS_remote, server_info)) {
    LOG(ERROR) << " address" << FLAGS_remote << " not correct";
    return -2;
  }

  mainloop.SetLoopName("main");
  mainloop.Start();

  raw_router.reset(new net::Client(&mainloop, server_info));

  config.connections = 4;
  config.recon_interval = 1000;
  config.message_timeout = 5000;

  raw_router->Initialize(config);
  bool running_ = true;
  co_go &mainloop << [&]() {
    while (running_) {
      std::string content;
      std::cout << "send:";
      std::flush(std::cout);
      std::getline(std::cin, content);
      running_ = content != "quit";
      if (!running_) {
        continue;
      }
      if (server_info.protocol == "raw") {
        DoLtRawRequest(content);
      } else {
        DoFwRapidRequest(content);
      }
    };
    raw_router->Finalize();
    mainloop.QuitLoop();
  };
  mainloop.WaitLoopEnd();
}
