#include <set>
#include <functional>
#include <glog/logging.h>

#include "net_io/tcp_channel.h"
#include "net_io/tcp_channel_ssl.h"
#include "net_io/socket_utils.h"
#include "net_io/socket_acceptor.h"
#include "net_io/base/ip_endpoint.h"

using namespace lt;

base::MessageLoop loop;
std::set<net::SocketChannelPtr> connections;

class EchoConsumer : public net::SocketChannel::Reciever {
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
};

EchoConsumer global_consumer;

DEFINE_bool(ssl, false, "use ssl");
DEFINE_string(cert, "cert/server.crt", "use ssl server cert file");
DEFINE_string(key, "cert/server.key", "use ssl server private key file");

int main(int argc, char** argv) {
  gflags::ParseCommandLineFlags(&argc, &argv, true);
  if (FLAGS_ssl) {
    LOG(INFO) << "ssl info\ncert:" << FLAGS_cert << "\nkey:" << FLAGS_key;
    lt::net::ssl_ctx_init(FLAGS_cert.data(), FLAGS_key.data());
  }

  loop.Start();

  auto on_new_connection = [&](int fd, const net::IPEndPoint& peer) {
    net::IPEndPoint local;
    CHECK(net::socketutils::GetLocalEndpoint(fd, &local));

    net::SocketChannelPtr ch;
    if (FLAGS_ssl) {
      ch = net::TCPSSLChannel::Create(fd, local, peer, loop.Pump());
      LOG(INFO) << "use ssl channel for this acceptor";
    } else {
      ch = net::TcpChannel::Create(fd, local, peer, loop.Pump());
      LOG(INFO) << "use tpc channel for this acceptor";
    }

    ch->SetReciever(&global_consumer);
    if (loop.IsInLoopThread()) {
      ch->StartChannel();
    } else {
      loop.PostTask(NewClosure(std::bind(&net::TcpChannel::StartChannel, ch.get())));
    }

    LOG(INFO) << "channel:[" << ch->ChannelName()
      << "] connected, connections count:" << connections.size();
    connections.insert(std::move(ch));
  };

  net::SocketAcceptor* acceptor;
  loop.PostTask(NewClosure([&]() {
    net::IPAddress addr;
    addr.AssignFromIPLiteral("127.0.0.1");
    net::IPEndPoint endpoint(addr, 5005);
    acceptor = new net::SocketAcceptor(loop.Pump(), endpoint);
    lt::net::NewConnectionCallback
      on_callback = std::bind(on_new_connection,
                              std::placeholders::_1,
                              std::placeholders::_2);
    acceptor->SetNewConnectionCallback(std::move(on_callback));
    CHECK(acceptor->StartListen());
    LOG(INFO) << "listen on:" << endpoint.ToString();
  }));

  loop.WaitLoopEnd();
  return 0;
}
