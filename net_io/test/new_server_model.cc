#include <vector>
#include "base/message_loop/message_loop.h"
#include "protocol/redis/redis_request.h"
#include "server/raw_server/raw_server.h"
#include "server/http_server/http_server.h"
#include "dispatcher/coro_dispatcher.h"
#include "dispatcher/workload_dispatcher.h"
#include "coroutine/coroutine_runner.h"

#include "clients/client_connector.h"
#include "clients/client.h"

#include <csignal>

using namespace lt::net;
using namespace lt;
using namespace base;

#define ENBALE_RAW_CLIENT
#define ENBALE_RDS_CLIENT
#define USE_CORO_DISPATCH 1

void DumpRedisResponse(RedisResponse* redis_response);
net::ClientConfig DeafaultClientConfig(int count = 2) {
  net::ClientConfig config;
  config.connections = count;
  config.recon_interval = 500;
  config.message_timeout = 1000;
  config.heartbeat_ms = 500;
  return config;
}

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

#ifdef USE_CORO_DISPATCH
    dispatcher_ = new net::CoroDispatcher(true);
#else
    dispatcher_ = new net::Dispatcher(true);
#endif
    dispatcher_->SetWorkerLoops(loops);
  }

  ~SampleApp() {
    delete dispatcher_;
#ifdef ENBALE_RAW_CLIENT
    delete raw_client;
#endif
#ifdef ENBALE_RDS_CLIENT
    delete redis_client;
#endif
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

  void StartHttpClients(std::string url) {
    net::url::RemoteInfo server_info;
    LOG_IF(ERROR, !net::url::ParseRemote(url, server_info)) << " server can't be resolve";
    http_client = new net::Client(NextIOLoopForClient(), server_info);
    net::ClientConfig config = DeafaultClientConfig(2);
    http_client->SetDelegate(this);
    http_client->Initialize(config);
  }

  void SendHttpRequest() {
    auto http_request = std::make_shared<HttpRequest>();
    http_request->SetMethod("GET");
    http_request->SetRequestURL("/ping");
    http_request->SetKeepAlive(true);

    HttpResponse* http_response = http_client->SendRecieve(http_request);
    if (http_response) {
      LOG(INFO) << "http client got response:" << http_response->Body();
    } else {
      LOG(ERROR) << "http client request failed:";
    }
  }

#ifdef ENBALE_RDS_CLIENT
  void StartRedisClient() {
    net::url::RemoteInfo server_info;
    LOG_IF(ERROR, !net::url::ParseRemote("redis://127.0.0.1:6379", server_info)) << " server can't be resolve";
    redis_client = new net::Client(NextIOLoopForClient(), server_info);
    net::ClientConfig config = DeafaultClientConfig();
    redis_client->SetDelegate(this);
    redis_client->Initialize(config);
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

    redis_request->TTL("counter");
    redis_request->Expire("counter", 200);

    redis_request->Persist("counter");
    redis_request->TTL("counter");

    auto redis_response  = redis_client->SendRecieve(redis_request);
    if (redis_response) {
      DumpRedisResponse(redis_response);
    } else {
      LOG(ERROR) << "redis client request failed:" << redis_request->FailCode();
    }
  }
#endif

#ifdef ENBALE_RAW_CLIENT
  void StartRawClient(std::string server_addr) {
    net::url::RemoteInfo server_info;
    LOG_IF(ERROR, !net::url::ParseRemote(server_addr, server_info)) << " server can't be resolve";
    raw_client = new net::Client(NextIOLoopForClient(), server_info);
    net::ClientConfig config = DeafaultClientConfig();
    raw_client->SetDelegate(this);
    raw_client->Initialize(config);
  }

  void SendRawRequest() {
    auto raw_request = LtRawMessage::Create(true);
    raw_request->SetMethod(12);
    raw_request->SetContent("ABC");
    LtRawMessage* raw_response = raw_client->SendRecieve(raw_request);
    if (!raw_response) {
      LOG(ERROR) << "raw client request failed:" << raw_request->FailCode();
    }
  }
#endif

  void StopAllService() {
    LOG(INFO) << __FUNCTION__ << " stop enter";

    LOG(INFO) << __FUNCTION__ << " start stop httpclient";
    http_client->Finalize();
    LOG(INFO) << __FUNCTION__ << " http client has stoped";

#ifdef ENBALE_RAW_CLIENT
    LOG(INFO) << __FUNCTION__ << " start stop rawclient";
    raw_client->Finalize();
    LOG(INFO) << __FUNCTION__ << " raw client has stoped";
#endif

#ifdef ENBALE_RDS_CLIENT
    LOG(INFO) << __FUNCTION__ << " start stop rdsclient";
    redis_client->Finalize();
    LOG(INFO) << __FUNCTION__ << " redis client has stoped";
#endif

    main_loop.QuitLoop();
    LOG(INFO) << __FUNCTION__ << " stop leave";
  }


#ifdef ENBALE_RAW_CLIENT
  net::Client*  raw_client = NULL; //(base::MessageLoop*, const SocketAddr&);
#endif
#ifdef ENBALE_RDS_CLIENT
  net::Client*  redis_client = NULL;
#endif

  net::Client*  http_client = NULL; //(base::MessageLoop*, const SocketAddr&);
  static std::atomic_int io_round_count;
  std::vector<base::MessageLoop*> loops;

#ifdef USE_CORO_DISPATCH
  CoroDispatcher* dispatcher_ = NULL;
#else
  Dispatcher* dispatcher_ = NULL;
#endif
};
std::atomic_int SampleApp::io_round_count = {0};


SampleApp app;

void HandleHttp(net::HttpContext* context) {
  net::HttpRequest* req = context->Request();
  LOG_EVERY_N(INFO, 10000) << " got 1w Http request, body:" << req->Dump();

#ifdef ENBALE_RAW_CLIENT
  if (req->RequestUrl() == "/raw") {
    app.SendRawRequest();
    return context->ReplyString("do raw request.");
  }
#endif

#ifdef ENBALE_RDS_CLIENT
  if (req->RequestUrl() == "/rds") {
    app.SendRedisMessage();
    return context->ReplyString("do rds request.");
  }
#endif

  if (req->RequestUrl() == "/http") {
    app.SendHttpRequest();
    return context->ReplyString("do http request.");
  } else if (req->RequestUrl() == "/ping") {
    return context->ReplyString("pong");
  }
  static const std::string kresponse(3650, 'c');
  context->ReplyString(kresponse);
}

void HandleRaw(net::RawServerContext* context) {
  const LtRawMessage* req = context->GetRequest<LtRawMessage>();
  LOG_EVERY_N(INFO, 10000) << " got request:" << req->Dump();

  auto res = LtRawMessage::CreateResponse(req);
  res->SetMethod(req->Method());
  res->SetContent(req->Content());
  return context->SendResponse(res);
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

void signalHandler( int signum ){
  LOG(INFO) << "sighandler sig:" << signum;
  app.StopAllService();
}

int main(int argc, char* argv[]) {
  gflags::ParseCommandLineFlags(&argc, &argv, true);

  main_loop.SetLoopName("main");
  main_loop.Start();

  net::RawServer raw_server;
  net::RawServer* rserver = &raw_server;
  raw_server.SetIOLoops(app.loops);
  raw_server.SetDispatcher(app.dispatcher_);
  raw_server.ServeAddressSync("raw://0.0.0.0:5005",
                              std::bind(HandleRaw, std::placeholders::_1));

  net::HttpServer http_server;
  net::HttpServer* hserver = &http_server;
  http_server.SetIOLoops(app.loops);
  http_server.SetDispatcher(app.dispatcher_);
  http_server.ServeAddressSync("http://0.0.0.0:5006",
                               std::bind(HandleHttp, std::placeholders::_1));

  app.StartHttpClients("http://127.0.0.1:5006");

#ifdef ENBALE_RAW_CLIENT
  app.StartRawClient("raw://127.0.0.1:5005");
#endif

#ifdef ENBALE_RDS_CLIENT
  app.StartRedisClient();
#endif

  signal(SIGINT, signalHandler);
  signal(SIGTERM, signalHandler);

  main_loop.WaitLoopEnd();

  rserver->StopServerSync();
  hserver->StopServerSync();

  std::this_thread::sleep_for(std::chrono::seconds(5));
}

