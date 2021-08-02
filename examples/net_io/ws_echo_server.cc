#include <csignal>
#include <vector>

#include "base/utils/string/str_utils.h"
#include "fmt/chrono.h"
#include "fmt/format.h"
#include "gflags/gflags.h"
#include "net_io/server/ws_server/ws_server.h"
#include "nlohmann/json.hpp"

using namespace lt::net;
using namespace lt;
using namespace base;

DEFINE_int32(loops, 4, "how many loops use for handle message and io");
DEFINE_string(addr, "0.0.0.0:5007", "host:port listen on");

class WSEchoService : public WSService {
public:
  ~WSEchoService() {
    LOG(INFO) << __func__ << " close app";
    for (auto loop : loops) {
      delete loop;
    }
    loops.clear();
  }

  void OnOpen(Websocket* ws) {
    LOG(INFO) << __func__ << " add to list";
    conns_.Append(ws);
  }

  void OnClose(Websocket* ws) {
    LOG(INFO) << __func__ << " remove from list";
    conns_.Remove(ws);
  }

  void OnMessage(Websocket* ws, const RefWebsocketFrame& message) {
    LOG(INFO) << __func__ << " message:" << message->Dump();
    ws->Send(message);
  }

  void Run() {
    int loop_count =
        std::max(FLAGS_loops, int(std::thread::hardware_concurrency()));
    LOG(INFO) << __func__ << " use loop count:" << loop_count;

    for (int i = 0; i < loop_count; i++) {
      loops.push_back(new base::MessageLoop(fmt::format("io_{}", i)));
      loops.back()->Start();
      CoroRunner::RegisteRunner(loops.back());
    }

    std::string address = base::StrUtil::Concat("ws://", FLAGS_addr);

    wss.WithIOLoops(loops);
    wss.Register("/chat", this);

    wss.ServeAddress(address);

    loops.back()->WaitLoopEnd();
  }

  void Stop() {
    CHECK(CO_CANYIELD);
    LOG(INFO) << __FUNCTION__ << " stop enter";
    wss.StopServer(CO_RESUMER);
    CO_YIELD;
    LOG(INFO) << __FUNCTION__ << " stop leave";
    loops.back()->QuitLoop();
  }

  WsServer wss;

  base::MessageLoop main_loop;

  nlohmann::json json_message;

  std::vector<base::MessageLoop*> loops;

  base::LinkedList<Websocket> conns_;
};

WSEchoService app;

void signalHandler(int signum) {
  LOG(INFO) << "sighandler sig:" << signum;
  CO_GO std::bind(&WSEchoService::Stop, &app);
}

int main(int argc, char* argv[]) {
  gflags::ParseCommandLineFlags(&argc, &argv, true);
  gflags::SetUsageMessage("usage: exec --addr=ip:port ");

  signal(SIGINT, signalHandler);
  signal(SIGTERM, signalHandler);

  app.Run();
}
