#include <csignal>
#include <vector>

#include "fmt/chrono.h"
#include "fmt/format.h"
#include "gflags/gflags.h"
#include "nlohmann/json.hpp"

#include "net_io/server/http_server/http_server.h"

#ifdef LTIO_HAVE_SSL
#include "base/crypto/lt_ssl.h"
#include "net_io/codec/http//h2/h2_tls_context.h"

DEFINE_bool(ssl, false, "using ssl socket");
DEFINE_string(key, "cert/server.key", "use ssl server key file");
DEFINE_string(cert, "cert/server.crt", "use ssl server cert file");

base::SSLCtx* server_ssl_ctx = nullptr;
#endif

using namespace lt::net;
using namespace lt;
using namespace base;
using namespace co;

DEFINE_bool(coro, false, "using coro context handle request");
DEFINE_string(addr, "0.0.0.0:5006", "host:port listen on");

class Http2Service {
public:
  ~Http2Service() {
    LOG(INFO) << __func__ << " close app";
  }

  void Run() {
    json_message["message"] = "Hello, World!";

    int loop_count = std::thread::hardware_concurrency();
    LOG(INFO) << __func__ << " use loop count:" << loop_count;
    loops = base::NewLoopBundles("io", loop_count);
    if (FLAGS_coro) {
      for (auto& loop : loops) {
        CoroRunner::RegisteRunner(loop.get());
      }
    }
#ifdef LTIO_HAVE_SSL
    if (FLAGS_ssl) {
      LOG(INFO) << "init ssl use cert:" << FLAGS_cert << ", key:" << FLAGS_key;
      base::SSLCtx::Param param(base::SSLRole::Server);
      param.crt_file = FLAGS_cert;
      param.key_file = FLAGS_key;
      server_ssl_ctx = new base::SSLCtx(param);
      CHECK(lt::net::configure_server_tls_context_easy(server_ssl_ctx->NativeHandle()));
    }
#endif

    auto func = [this](const RefH2Context& context) {
      const HttpRequest* req = context->Request();
      VLOG(VTRACE) << "got:" << req->Dump();

      auto response = HttpResponse::CreateWithCode(200);
      response->SetKeepAlive(req->IsKeepAlive());

      response->InsertHeader("Server", "ltio");
      auto tm = fmt::gmtime(std::time(nullptr));
      response->InsertHeader("Date",
                             fmt::format("{:%a, %d %b %Y %H:%M:%S %Z}", tm));
      if (req->RequestUrl() == "/push") {
        auto promise_res = HttpResponse::CreateWithCode(200);
        promise_res->SetKeepAlive(req->IsKeepAlive());
        promise_res->InsertHeader("Server", "ltio");
        promise_res->SetBody("promise body come");
        context->Push("GET", "/css", req, promise_res, [](int code) {
          LOG(INFO) << "server push result:" << code;
        });
      } else if (req->RequestUrl() == "/plaintext") {

        response->MutableBody() = "Hello, World!";

      } else if (req->RequestUrl() == "/json") {

        response->InsertHeader("Content-Type", "application/json");
        response->MutableBody() = std::move(json_message.dump());

      }
      return context->Response(response);
    };

    handler.reset(FLAGS_coro ? NewHttp2CoroHandler(func)
                             : NewHttp2Handler(func));

    std::string address = "h2c://" + FLAGS_addr;
    http_server.WithIOLoops(base::RawLoopsFromBundles(loops));
#ifdef LTIO_HAVE_SSL
    if (FLAGS_ssl) {
      address = "h2://" + FLAGS_addr;
      http_server.WithSSLContext(server_ssl_ctx);
    }
#endif

    http_server.WithAddress(address).ServeAddress(handler.get());
    loops.back()->WaitLoopEnd();
  }

  void Stop() {
    CHECK(CO_CANYIELD);
    LOG(INFO) << __FUNCTION__ << " stop enter";
    http_server.StopServer(CO_RESUMER);
    CO_YIELD;
    LOG(INFO) << __FUNCTION__ << " stop leave";
    loops.back()->QuitLoop();
  }

  HttpServer http_server;
  std::unique_ptr<CodecService::Handler> handler;
  base::MessageLoop main_loop;
  nlohmann::json json_message;
  base::RefLoopList loops;
};

Http2Service app;
void signalHandler(int signum) {
  LOG(INFO) << "handle signal:" << signum;
  CO_GO std::bind(&Http2Service::Stop, &app);
}

int main(int argc, char* argv[]) {
  gflags::ParseCommandLineFlags(&argc, &argv, true);
  gflags::SetUsageMessage("usage: exec --addr=ip:port [--coro --ssl --key= --cert= ]");

  signal(SIGINT, signalHandler);
  signal(SIGTERM, signalHandler);

  app.Run();
}
