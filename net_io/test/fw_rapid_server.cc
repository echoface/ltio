#include <vector>
#include "base/message_loop/message_loop.h"
#include "server/raw_server/raw_server.h"
#include "dispatcher/coro_dispatcher.h"
#include "dispatcher/workload_dispatcher.h"
#include "coroutine/coroutine_runner.h"
#include "protocol/raw/raw_proto_service.h"

#include "clients/client_connector.h"
#include "clients/client.h"

#include "fw_rapid_message.h"

#include <csignal>

using namespace lt::net;
using namespace lt;
using namespace base;

#define USE_CORO_DISPATCH 1

base::MessageLoop main_loop;
class SampleApp: public net::ClientDelegate {
public:
  SampleApp() {
    int loop_count = std::min(4, int(std::thread::hardware_concurrency()));
    for (int i = 0; i < loop_count; i++) {
      auto loop = new(base::MessageLoop);
      loop->SetLoopName("io_" + std::to_string(i));
      loop->Start();
      loops.push_back(loop);
    }

    dispatcher_ = new net::CoroDispatcher(true);
    dispatcher_->SetWorkerLoops(loops);
  }

  ~SampleApp() {
    delete dispatcher_;
    for (auto loop : loops) {
      delete loop;
    }
    loops.clear();
  }

  base::MessageLoop* NextIOLoopForClient() {
    if (loops.empty()) {
      return NULL;
    }
    io_round_count++;
    return loops[io_round_count % loops.size()];
  }

  void StopAllService() {
    LOG(INFO) << __FUNCTION__ << " stop enter";
    main_loop.QuitLoop();
    LOG(INFO) << __FUNCTION__ << " stop leave";
  }

  static std::atomic_int io_round_count;
  std::vector<base::MessageLoop*> loops;
  CoroDispatcher* dispatcher_ = NULL;
};

std::atomic_int SampleApp::io_round_count = {0};

SampleApp app;

void HandleRaw(net::RawServerContext* context) {
  const FwRapidMessage* req = context->GetRequest<FwRapidMessage>();
  LOG_EVERY_N(INFO, 10000) << " got request:" << req->Dump();
  auto res = FwRapidMessage::CreateResponse(req);
  res->SetContent(req->Content());
  return context->SendResponse(res);
}

void signalHandler( int signum ){
  LOG(INFO) << "sighandler sig:" << signum;
  app.StopAllService();
}

int main(int argc, char* argv[]) {
  gflags::ParseCommandLineFlags(&argc, &argv, true);

  main_loop.SetLoopName("main");
  main_loop.Start();

  net::ProtoServiceFactory::Instance().RegisterCreator("rapid", []() -> net::RefProtoService {
    auto service = std::make_shared<RawProtoService<FwRapidMessage>>();
    return std::static_pointer_cast<ProtoService>(service);
  });

  net::RawServer raw_server;
  net::RawServer* rserver = &raw_server;
  raw_server.SetIOLoops(app.loops);
  raw_server.SetDispatcher(app.dispatcher_);
  raw_server.ServeAddressSync("rapid://0.0.0.0:5004",
                              std::bind(HandleRaw, std::placeholders::_1));

  signal(SIGINT, signalHandler);
  signal(SIGTERM, signalHandler);

  main_loop.WaitLoopEnd();
  rserver->StopServerSync();
  std::this_thread::sleep_for(std::chrono::seconds(5));
}


