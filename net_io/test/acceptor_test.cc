#include <set>
#include <functional>
#include <glog/logging.h>

#include "../tcp_channel.h"
#include "../socket_utils.h"
#include "../socket_acceptor.h"

using namespace lt;

base::MessageLoop loop;
std::set<net::RefTcpChannel> connections;

class EchoConsumer : public net::ChannelConsumer {
public:
  void OnChannelClosed(const net::RefTcpChannel& ch) override {
    connections.erase(ch);
    LOG(INFO) << "channel:[" << ch->ChannelName() << "] closed, connections count:" << connections.size();
  };

  void OnDataReceived(const net::RefTcpChannel &ch, net::IOBuffer *buf) override {
    ch->Send(buf->GetRead(), buf->CanReadSize());
    buf->Consume(buf->CanReadSize());
  };
};

EchoConsumer global_consumer;

int main(int argc, char** argv) {

  loop.Start();

  auto on_new_connection = [&](int fd, const net::SocketAddress& peer) {
    net::SocketAddress local(net::socketutils::GetLocalAddrIn(fd));
    auto ch = net::TcpChannel::Create(fd, local, peer, &loop);
    ch->SetChannelConsumer(&global_consumer);
    ch->Start();

    connections.insert(ch);
    LOG(INFO) << "channel:[" << ch->ChannelName() << "] connected, connections count:" << connections.size();
  };

  net::SocketAcceptor* acceptor;
  loop.PostTask(NewClosure([&]() {

    net::SocketAddress addr(5005);
    acceptor = new net::SocketAcceptor(loop.Pump(), addr);

    acceptor->SetNewConnectionCallback(std::bind(on_new_connection, std::placeholders::_1, std::placeholders::_2));
    CHECK(acceptor->StartListen());
  }));


  loop.WaitLoopEnd();
  return 0;
}
