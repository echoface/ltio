#include <memory>
#include <atomic>
#include <functional>
#include <glog/logging.h>
#include <gflags/gflags.h>
#include "net_io/clients/client.h"
#include "net_io/protocol/raw/raw_proto_service.h"
#include "net_io/protocol/proto_service_factory.h"

#include "base/coroutine/wait_group.h"
#include "base/coroutine/coroutine_runner.h"

#include "fw_rapid_message.h"

using namespace lt;

static const int connections = 10;

std::vector<base::MessageLoop*> InitLoop(int n) {
  std::vector<base::MessageLoop*> lops;
  for (int i = 0; i < n; i++) {
    base::MessageLoop* l = new base::MessageLoop();
    l->SetLoopName("io_" + std::to_string(i));
    l->Start();
    lops.push_back(l);
  }
  return lops;
}

std::vector<base::MessageLoop*> loops;
static std::atomic_int io_round_count;
class SampleApp: public net::ClientDelegate{
  public:
    SampleApp() {
      net::ProtoServiceFactory::Instance().RegisterCreator("rapid", []() -> net::RefProtoService {
        auto service = std::make_shared<net::RawProtoService<FwRapidMessage>>();
        return std::static_pointer_cast<net::ProtoService>(service);
      });
    }

    base::MessageLoop* NextIOLoopForClient() {
      if (loops.empty()) {
        return NULL;
      }
      io_round_count++;
      return loops[io_round_count % loops.size()];
    }
};

DEFINE_int32(c, 4, "concurrency threads count");
DEFINE_int32(n, 5000,"benchmark request numbers");
DEFINE_string(url, "", "remote rapid server url, eg:rapid://127.0.0.1:5004");

int main(int argc, char** argv) {
  gflags::ParseCommandLineFlags(&argc, &argv, false);

  int concurency = FLAGS_c;
  int req_count = FLAGS_n;
  std::string url = FLAGS_url;
  LOG(INFO) << "c:" << concurency << " n:" << req_count << " url:" << url;
  if (url.empty()) return -1;

  SampleApp app;
  loops = InitLoop(4);

  base::MessageLoop loop;
  loop.SetLoopName("client");
  loop.Start();


  net::url::RemoteInfo server_info;
  LOG_IF(ERROR, !net::url::ParseRemote(url, server_info)) << " server can't be resolve";
  net::Client raw_router(&loop, server_info);

  net::ClientConfig config;
  config.recon_interval = 1000;
  config.message_timeout = 5000;
  config.connections = connections;

  raw_router.SetDelegate(&app);
  raw_router.Initialize(config);

  std::atomic_int64_t  total_task;
  total_task = req_count;

  std::atomic_int64_t failed_request;
  failed_request = 0;
  std::atomic_int64_t success_request;
  success_request = 0;

  base::WaitGroup wc;

  auto raw_request_task = [&]() {
    while(total_task.fetch_sub(1) > 0) {

      auto request = FwRapidMessage::Create(true);
      request->SetCmdId(12);
      request->SetContent("RawRequest");

      FwRapidMessage* response = raw_router.SendRecieve(request);
      if (response && request->FailCode() == net::MessageCode::kSuccess) {
        success_request++;
      } else {
        failed_request++;
      }
      if (total_task <= 0) {
        break;
      }
    };
    wc.Done();
  };

  sleep(3);
  LOG(INFO) << " start bench started.............<<<<<<";
  for (int i = 0; i < concurency; i++) {
    wc.Add(1);
    base::MessageLoop* loop = loops[i % loops.size()];
    co_go loop << raw_request_task;
  }

  co_go &loop << [&]() {
    wc.Wait();
    LOG(INFO) << "success:" << success_request << " failed:" << failed_request;
    raw_router.Finalize();
    loop.QuitLoop();
  };
  loop.WaitLoopEnd();
  for (auto loop : loops) {
    delete loop;
  }
  loops.clear();
  LOG(INFO) << " main loop quit";
  std::this_thread::sleep_for(std::chrono::seconds(2));
}

