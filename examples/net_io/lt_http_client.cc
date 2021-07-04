#include <memory>
#include <atomic>
#include <functional>
#include <glog/logging.h>
#include <gflags/gflags.h>
#include "net_io/clients/client.h"
#include "base/coroutine/coroutine_runner.h"
#include "net_io/codec/http/http_request.h"
#include "net_io/codec/http/http_response.h"

using namespace lt;
using base::MessageLoop;

typedef std::unique_ptr<net::Client> ClientPtr;
ClientPtr http_client;

DEFINE_string(cert, "", "cert ca file");
DEFINE_string(remote, "", "scheme://host:port remote address");

int main(int argc, char** argv) {
  gflags::ParseCommandLineFlags(&argc, &argv, true);
  gflags::SetUsageMessage("usage: ./lt_client_cli --remote=scheme://host:port[?[supported queries]]");

  if (FLAGS_remote.empty()) {
    gflags::ShowUsageWithFlags(argv[0]);
    return -1;
  }

  base::MessageLoop mainloop;

  net::ClientConfig config;
  net::url::RemoteInfo server_info;
  if (!net::url::ParseRemote(FLAGS_remote, server_info)) {
    LOG(ERROR) << " address" << FLAGS_remote << " not correct";
    return -2;
  }

  mainloop.SetLoopName("main");
  mainloop.Start();

  http_client.reset(new net::Client(&mainloop, server_info));

  config.connections = 1;
  config.recon_interval = 1000;
  config.message_timeout = 5000;
  http_client->Initialize(config);


#ifdef LTIO_HAVE_SSL
  auto client_ssl_ctx = new base::SSLCtx(base::SSLRole::Client);
  client_ssl_ctx->UseCertification(FLAGS_cert, "");
  http_client->SetSSLCtx(client_ssl_ctx);
#endif

  while (true) {
    std::string content;
    std::cout << "next|quit";
    std::flush(std::cout);
    std::getline(std::cin, content);
    if (content == "quit") {
      break;
    }
    if (content != "next") {
      continue;
    }
    auto http_request = std::make_shared<lt::net::HttpRequest>();
    http_request->SetMethod("GET");
    http_request->SetRequestURL("/ping");
    http_request->SetKeepAlive(true);

    bool success = http_client->AsyncDoRequest(http_request, [](lt::net::CodecMessage* res) {
      if (res == nullptr) {
        LOG(INFO) << "nil response";
      } else {
        LOG(INFO) << "response:" << res->Dump();
      }
    });
    LOG(INFO) << "send http result:" << success;
  };
  http_client->Finalize();
  mainloop.QuitLoop();
  mainloop.WaitLoopEnd();
}

