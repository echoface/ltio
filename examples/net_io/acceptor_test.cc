#include <set>
#include <functional>
#include <glog/logging.h>

#include "net_io/tcp_channel.h"
#include "net_io/socket_utils.h"
#include "net_io/socket_acceptor.h"
#include "net_io/base/ip_endpoint.h"

using namespace lt;

base::MessageLoop loop;
std::set<net::RefTcpChannel> connections;

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
    LOG(INFO) << "channel:[" << ch->ChannelName() << "] closed, connections count:" << connections.size();
  };

  void OnDataReceived(const net::SocketChannel* ch, net::IOBuffer *buf) override {
    auto iter = std::find_if(connections.begin(),
                             connections.end(),
                             [&](const net::RefTcpChannel& channel) -> bool {
                               return ch == channel.get();
                             });
    net::TcpChannel* channel = iter->get();
    channel->Send(buf->GetRead(), buf->CanReadSize());
    buf->Consume(buf->CanReadSize());
  };
};

EchoConsumer global_consumer;

int main(int argc, char** argv) {

  loop.Start();

  auto on_new_connection = [&](int fd, const net::IPEndPoint& peer) {
    net::IPEndPoint local;
    CHECK(net::socketutils::GetLocalEndpoint(fd, &local));

    auto ch = net::TcpChannel::Create(fd, local, peer, loop.Pump());
    ch->SetReciever(&global_consumer);

    if (loop.IsInLoopThread()) {
      ch->StartChannel();
    } else {
      loop.PostTask(NewClosure(std::bind(&net::TcpChannel::StartChannel, ch)));
    }

    connections.insert(ch);
    LOG(INFO) << "channel:[" << ch->ChannelName() << "] connected, connections count:" << connections.size();
  };

  net::SocketAcceptor* acceptor;
  loop.PostTask(NewClosure([&]() {
    net::IPAddress addr;
    addr.AssignFromIPLiteral("127.0.0.1");
    net::IPEndPoint endpoint(addr, 5005);
    acceptor = new net::SocketAcceptor(loop.Pump(), endpoint);
    acceptor->SetNewConnectionCallback(std::bind(on_new_connection, std::placeholders::_1, std::placeholders::_2));
    CHECK(acceptor->StartListen());
  }));

  loop.WaitLoopEnd();
  return 0;
}
