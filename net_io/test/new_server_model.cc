#include <vector>
#include "base/message_loop/message_loop.h"
#include "protocol/redis/redis_request.h"
#include "server/raw_server/raw_server.h"
#include "server/http_server/http_server.h"
#include "dispatcher/coro_dispatcher.h"
#include "dispatcher/workload_dispatcher.h"
#include "coroutine/coroutine_runner.h"

#include "clients/client_connector.h"
#include "clients/client_router.h"

using namespace net;
using namespace base;

#define ENBALE_RAW_CLIENT

void SendRawRequest();

base::MessageLoop main_loop;
std::vector<base::MessageLoop*> loops;
std::vector<base::MessageLoop*> workers;

CoroDispatcher* dispatcher_ = new CoroDispatcher(true);
Dispatcher* common_dispatcher = new Dispatcher(true);

std::atomic_int64_t http_count;
static const std::string kresponse(3650, 'c');

void HandleHttp(net::HttpContext* context) {
  net::HttpRequest* req = context->Request();

  LOG_EVERY_N(INFO, 10000) << " got 1w Http request, body:" << req->Dump();
  http_count++;

#ifdef ENBALE_RAW_CLIENT
  if (req->RequestUrl() == "/br") {
      SendRawRequest();
  }
#endif
  context->ReplyString(kresponse);
}

void HandleRaw(const LtRawMessage* req, LtRawMessage* res) {
  LOG_EVERY_N(INFO, 10000) << " got 1w Raw request" << req->Dump();

  res->MutableHeader()->code = 0;
  res->MutableHeader()->method = 2;
  res->SetContent("Raw Message");
}

net::ClientRouter*  raw_router; //(base::MessageLoop*, const SocketAddress&);
net::ClientRouter*  http_router; //(base::MessageLoop*, const SocketAddress&);
net::ClientRouter*  redis_router;

static std::atomic_int io_round_count;
class RouterManager: public net::RouterDelegate {
public:
  base::MessageLoop* NextIOLoopForClient() {
    if (loops.empty()) {
      return NULL;
    }
    io_round_count++;
    return loops[io_round_count % loops.size()];
  }
};

RouterManager router_manager;

void StartRedisClient() {
  net::url::SchemeIpPort server_info;
  LOG_IF(ERROR, !net::url::ParseURI("redis://127.0.0.1:6379", server_info)) << " server can't be resolve";

  redis_router = new net::ClientRouter(&main_loop, server_info);
  net::RouterConf router_config;
  router_config.connections = 2;
  router_config.recon_interval = 5000;
  router_config.message_timeout = 1000;
  redis_router->SetupRouter(router_config);
  redis_router->SetDelegate(&router_manager);

  redis_router->StartRouter();
}

void StartRawClient() {
  net::url::SchemeIpPort server_info;
  LOG_IF(ERROR, !net::url::ParseURI("raw://127.0.0.1:5005", server_info)) << " server can't be resolve";

  raw_router = new net::ClientRouter(&main_loop, server_info);
  net::RouterConf router_config;
  router_config.connections = 4;
  router_config.recon_interval = 100;
  router_config.message_timeout = 1000;
  raw_router->SetupRouter(router_config);
  raw_router->SetDelegate(&router_manager);

  raw_router->StartRouter();
}

void StartHttpClients() {
  net::url::SchemeIpPort server_info;
  LOG_IF(ERROR, !net::url::ParseURI("http://127.0.0.1:5006", server_info)) << " server can't be resolve";
  http_router = new net::ClientRouter(&main_loop, server_info);
  net::RouterConf router_config;
  router_config.connections = 2;
  router_config.recon_interval = 100;
  router_config.message_timeout = 1000;
  http_router->SetupRouter(router_config);
  http_router->SetDelegate(&router_manager);

  http_router->StartRouter();
}

void SendHttpRequest() {
  auto http_request = std::make_shared<HttpRequest>();
  http_request->SetMethod("GET");
  http_request->SetRequestURL("/abc/list");
  http_request->SetKeepAlive(true);

  HttpResponse* http_response = http_router->SendRecieve(http_request);
  if (http_response) {
    LOG(INFO) << "http client got response:" << http_response->Body();
  } else {
    LOG(ERROR) << "http client request failed:";
  }
}

void SendRawRequest() {
  auto raw_request = LtRawMessage::Create(true);
  raw_request->MutableHeader()->method = 12;
  raw_request->SetContent("ABC");

  LtRawMessage* raw_response = raw_router->SendRecieve(raw_request);
  if (!raw_response) {
    LOG(ERROR) << "raw client request failed:" << raw_request->FailCode();
  }
}

void DumpRedisResponse(RedisResponse* redis_response) {
  for (size_t i = 0; i < redis_response->Count(); i++) {

    auto& value = redis_response->ResultAtIndex(i);
    switch(value.type()) {
      case resp::ty_string: {
        LOG(INFO) << "string:" << value.string().data();
      }
      break;
      case resp::ty_error: {
        LOG(INFO) << "error:" << value.error().data();
      }
      break;
      case resp::ty_integer: {
        LOG(INFO) << "interger:" << value.integer();
      }
      break;
      case resp::ty_null: {
        LOG(INFO) << "null:";
      } break;
      case resp::ty_array: {
        resp::unique_array<resp::unique_value> arr = value.array();
        std::ostringstream oss;
        for (size_t i = 0; i < arr.size(); i++) {
          oss << "'" << std::string(arr[i].bulkstr().data(), arr[i].bulkstr().size()) << "'";
          if (i < arr.size() - 1) {
            oss << ", ";
          }
        }
        LOG(INFO) << "array: [" << oss.str() << "]";
      } break;
      case resp::ty_bulkstr: {
        LOG(INFO) << "bulkstr:" << std::string(value.bulkstr().data(), value.bulkstr().size());
      }
      break;
      default: {
        LOG(INFO) << " default handler for redis message response";
      }
      break;
    }
  }
}

void SendRedisMessage() {
  auto redis_request = std::make_shared<RedisRequest>();

  redis_request->SetWithExpire("name", "huan.gong", 2000);
  redis_request->Exists("name");
  redis_request->Delete("name");

  redis_request->Incr("counter");
  redis_request->IncrBy("counter", 10);
  redis_request->Decr("counter");
  redis_request->DecrBy("counter", 10);

  redis_request->Select("1");
  redis_request->Auth("");

  redis_request->TTL("counter");
  redis_request->Expire("counter", 200);
  redis_request->Persist("counter");
  redis_request->TTL("counter");

  auto redis_response  = redis_router->SendRecieve(redis_request);
  if (redis_response) {
    DumpRedisResponse(redis_response);
  } else {
    LOG(ERROR) << "redis client request failed:" << redis_request->FailCode();
  }
}

void PrepareLoops(uint32_t io_count, uint32_t worker_count) {
  for (uint32_t i = 0; i < io_count; i++) {
    auto loop = new(base::MessageLoop);

    loop->SetLoopName("io_" + std::to_string(i));
    loop->Start();
    loops.push_back(loop);
  }

  for (uint32_t i = 0; i < worker_count; i++) {
    auto worker = new(base::MessageLoop);

    worker->SetLoopName("worker_" + std::to_string(i));
    worker->Start();
    workers.push_back(worker);
  }
}

int main(int argc, char* argv[]) {
  //google::ParseCommandLineFlags(&argc, &argv, true);  // 初始化 gflags

  main_loop.SetLoopName("main");
  main_loop.Start();
  http_count.store(0);
  PrepareLoops(std::thread::hardware_concurrency(), 1);

  dispatcher_->SetWorkerLoops(loops);
  common_dispatcher->SetWorkerLoops(loops);

  net::RawServer raw_server;
  raw_server.SetIOLoops(loops);
  raw_server.SetDispatcher(dispatcher_);
  raw_server.ServeAddressSync("raw://0.0.0.0:5005", std::bind(HandleRaw, std::placeholders::_1, std::placeholders::_2));

  net::HttpServer http_server;
  http_server.SetIOLoops(loops);
  http_server.SetDispatcher(dispatcher_);
  http_server.ServeAddressSync("http://0.0.0.0:5006", std::bind(HandleHttp, std::placeholders::_1));

#if 0
  StartHttpClients();
#endif


#ifdef ENBALE_RAW_CLIENT
  StartRawClient();
#endif

#if 0
  std::this_thread::sleep_for(std::chrono::seconds(5));
  http_server.StopServerSync();
  raw_router->StopRouter();
#endif

#if 0
  StartRedisClient();
  std::this_thread::sleep_for(std::chrono::seconds(5));
  main_loop.PostCoroTask(std::bind(SendRedisMessage));
#endif

  main_loop.WaitLoopEnd();
}

