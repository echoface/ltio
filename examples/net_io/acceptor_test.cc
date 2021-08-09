#include <glog/logging.h>
#include <functional>
#include <set>

#include "net_io/base/ip_endpoint.h"
#include "net_io/socket_acceptor.h"
#include "net_io/socket_utils.h"
#include "net_io/tcp_channel.h"

#ifdef LTIO_HAVE_SSL
#include "net_io/tcp_channel_ssl.h"
DEFINE_bool(ssl, false, "use ssl");
DEFINE_string(cert, "cert/server.crt", "use ssl server cert file");
DEFINE_string(key, "cert/server.key", "use ssl server private key file");

base::SSLCtx* server_ssl_ctx = nullptr;
#endif

using namespace lt;

base::MessageLoop loop;
std::map<base::FdEvent*, net::SocketChannelPtr> connections;

class EchoConsumer : public net::SocketAcceptor::Actor,
                     public base::FdEvent::Handler {
public:
  void OnDataReceived(net::SocketChannel* ch, net::IOBuffer* buf) {
    CHECK(loop.IsInLoopThread());
    ignore_result(ch->Send(buf->GetRead(), buf->CanReadSize()));
    buf->Consume(buf->CanReadSize());
  };

  bool IsServerSide() const { return true; }

  void OnNewConnection(int fd, const net::IPEndPoint& peer) override {
    net::IPEndPoint local;
    CHECK(net::socketutils::GetLocalEndpoint(fd, &local));

    auto fdev = new base::FdEvent(this, fd, base::LT_EVENT_READ);

    net::SocketChannelPtr ch;
#ifdef LTIO_HAVE_SSL
    if (FLAGS_ssl) {
      auto ssl_ch = net::TCPSSLChannel::Create(fd, local, peer);
      ssl_ch->InitSSL(server_ssl_ctx->NewSSLSession(fd));
      LOG(INFO) << "use ssl channel for this acceptor";
    } else {
      ch = net::TcpChannel::Create(fd, local, peer);
      LOG(INFO) << "use tpc channel for this acceptor";
    }
#else
    ch = net::TcpChannel::Create(fd, local, peer);
#endif

    ch->SetFdEvent(fdev);
    auto ch_p = ch.get();
    loop.PostTask(FROM_HERE, [ch_p](){
      ignore_result(ch_p->StartChannel(true));
    });

    LOG(INFO) << "channel:[" << ch->ChannelInfo()
              << "] connected, connections count:" << connections.size();
    loop.Pump()->InstallFdEvent(fdev);
    connections[fdev] = std::move(ch);
  }

  void CloseChannel(base::FdEvent* fdev) {
    CHECK(loop.IsInLoopThread());
    LOG(INFO) << "remove this connections:" << fdev->EventInfo();
    loop.Pump()->RemoveFdEvent(fdev);
    connections.erase(fdev);
    delete fdev;
  }

  void HandleEvent(base::FdEvent* fdev) override {
    auto ch = connections[fdev].get();
    LOG(INFO) << "handle event:" << ch->ChannelInfo();
    if (!ch->HandleRead()) {
      LOG(INFO) << "handle event failed";
      return CloseChannel(fdev);
    }
    if (ch->ReaderBuffer()->CanReadSize()) {
      OnDataReceived(ch, ch->ReaderBuffer());
    }
  }
};

EchoConsumer global_consumer;

int main(int argc, char** argv) {
  gflags::ParseCommandLineFlags(&argc, &argv, true);
#ifdef LTIO_HAVE_SSL
  if (FLAGS_ssl) {
    LOG(INFO) << "ssl info\ncert:" << FLAGS_cert << "\nkey:" << FLAGS_key;
    server_ssl_ctx = new base::SSLCtx(base::SSLRole::Server);
    server_ssl_ctx->UseCertification(FLAGS_cert, FLAGS_key);
  }
#endif

  loop.Start();

  net::IPAddress addr;
  addr.AssignFromIPLiteral("127.0.0.1");
  net::IPEndPoint endpoint(addr, 5005);
  net::SocketAcceptor* acceptor =
      new net::SocketAcceptor(&global_consumer, loop.Pump(), endpoint);
  loop.PostTask(FROM_HERE, [&]() {
    CHECK(acceptor->StartListen());
    LOG(INFO) << "listen on:" << endpoint.ToString();
  });

  loop.WaitLoopEnd();
  return 0;
}
