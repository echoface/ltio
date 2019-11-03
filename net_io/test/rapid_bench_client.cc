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
#include "thirdparty/argparser/argparse.h"
#include "base/coroutine/wait_group.h"

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

static std::atomic_int io_round_count;
class RouterManager: public net::ClientDelegate{
  public:
    RouterManager() {
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

std::vector<base::MessageLoop*> loops;
int main(int argc, char** argv) {

  ArgumentParser parser(" rapid client bench client");
  parser.add_argument("-c", "concurrency count", true);
  parser.add_argument("-n", "bench request count", true);
  parser.add_argument("--url", "remote rapid server url", true);
  try {
    parser.parse(argc, argv);
  } catch (const ArgumentParser::ArgumentNotFound& ex) {
    LOG(ERROR) << " args parse failed:" << ex.what();
    return 0;
  }
  if (parser.is_help()) return 0;

  int concurency = parser.get<int>("c");
  int req_count = parser.get<int>("n");
  std::string url = parser.get<std::string>("url");
  LOG(INFO) << "c:" << concurency << " n:" << req_count << " url:" << url;

  base::MessageLoop loop;
  loop.SetLoopName("client");
  loop.Start();

  loops = InitLoop(concurency);
  RouterManager router_delegate;

  net::url::RemoteInfo server_info;
  LOG_IF(ERROR, !net::url::ParseRemote(url, server_info)) << " server can't be resolve";
  net::Client raw_router(&loop, server_info);


  net::ClientConfig config;
  config.recon_interval = 1000;
  config.message_timeout = 5000;
  config.connections = connections;

  raw_router.SetDelegate(&router_delegate);
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
    };
    wc.Done();
    LOG(INFO) << " request task run coro end";
  };

  sleep(3);
  LOG(INFO) << " start bench started.............<<<<<<";
  for (int i = 0; i < concurency; i++) {
    wc.Add(1);
    auto l = loops[i % loops.size()];
    co_go l << raw_request_task;
  }

  co_go &loop << [&]() {
    wc.Wait();
    raw_router.Finalize();
    loop.QuitLoop();
  };
  loop.WaitLoopEnd();
  LOG(INFO) << " main loop quit";
}

