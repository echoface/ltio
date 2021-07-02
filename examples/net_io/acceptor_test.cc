#include <set>
#include <functional>
#include <glog/logging.h>

#include "net_io/tcp_channel.h"
#include "net_io/socket_utils.h"
#include "net_io/socket_acceptor.h"
#include "net_io/base/ip_endpoint.h"

#ifdef LTIO_HAVE_SSL
#include "net_io/tcp_channel_ssl.h"
DEFINE_bool(ssl, false, "use ssl");
DEFINE_string(cert, "cert/server.crt", "use ssl server cert file");
DEFINE_string(key, "cert/server.key", "use ssl server private key file");

base::SSLCtx* server_ssl_ctx = nullptr;
#endif

using namespace lt;

base::MessageLoop loop;
std::set<net::SocketChannelPtr> connections;

class EchoConsumer :
  public net::SocketAcceptor::Actor,
  public net::SocketChannel::Reciever {
public:
  void OnChannelClosed(const net::SocketChannel* ch) override {
    auto iter = connections.begin();
    for (; iter != connections.end(); iter++) {
      if (iter->get() == ch) {
        iter = connections.erase(iter);
        break;
      }
    }
    LOG(INFO) << "channel:[" << ch->ChannelInfo()
      << "] closed, connections count:" << connections.size();
  };

  void OnDataReceived(const net::SocketChannel* ch, net::IOBuffer *buf) override {
    auto iter =
        std::find_if(connections.begin(), connections.end(),
                     [&](const net::SocketChannelPtr& channel) -> bool {
                       return ch == channel.get();
                     });

    (*iter)->Send(buf->GetRead(), buf->CanReadSize());
    buf->Consume(buf->CanReadSize());
  };

  bool IsServerSide() const override {return true;}

  void OnNewConnection(int fd, const net::IPEndPoint& peer) {
    net::IPEndPoint local;
    CHECK(net::socketutils::GetLocalEndpoint(fd, &local));

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
    ch->SetIOEventPump(loop.Pump());

    ch->SetReciever(this);
    if (loop.IsInLoopThread()) {
      ch->StartChannel();
    } else {
      loop.PostTask(NewClosure(std::bind(&net::TcpChannel::StartChannel, ch.get())));
    }

    LOG(INFO) << "channel:[" << ch->ChannelName()
      << "] connected, connections count:" << connections.size();
    connections.insert(std::move(ch));
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
  loop.PostTask(NewClosure([&]() {
    CHECK(acceptor->StartListen());
    LOG(INFO) << "listen on:" << endpoint.ToString();
  }));

  loop.WaitLoopEnd();
  return 0;
}
