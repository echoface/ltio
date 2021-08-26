#include "io_service.h"

#include "base/coroutine/co_runner.h"
#include "base/coroutine/io_event.h"
#include "base/utils/sys_error.h"
#include "net_io/socket_utils.h"
#include "net_io/codec/codec_factory.h"
#include "net_io/tcp_channel.h"

using lt::net::SockaddrStorage;
using namespace lt::net::socketutils;
using lt::net::TcpChannel;
using lt::net::SocketChannelPtr;

namespace coso {

IOService::IOService(const IPEndPoint& addr) : address_(addr) {}

void IOService::Run() {
  int socket = CreateNoneBlockTCP(address_.GetSockAddrFamily());
  CHECK(socket > 0);
  ReUseSocketPort(socket, true);
  ReUseSocketAddress(socket, true);

  SockaddrStorage storage;
  address_.ToSockAddr(storage.AsSockAddr(), storage.Size());

  int ret = BindSocketFd(socket, storage.AsSockAddr());
  CHECK(ret >= 0) << Name() << " bind socket fail, socket:" << socket;
  CHECK(ListenSocket(socket) >= 0)
      << "listen fail, reason:" << base::StrError();

  return accept_loop(socket);
}

void IOService::accept_loop(int socket) {
  base::FdEvent fdev(socket, base::LtEv::READ);
  co::IOEvent ioev(&fdev);
  do {
    ioev.Wait();
    LOG(INFO) << "accept loop awake";

    int err = 0;
    struct sockaddr socket_in;
    int peer_fd = AcceptSocket(socket, &socket_in, &err);
    if (peer_fd < 0) {
      LOG(ERROR) << "accept failed, err:" << base::StrError(err);
      continue;
    }
    IPEndPoint client_addr;
    client_addr.FromSockAddr(&socket_in, sizeof(socket_in));
    VLOG(VTRACE) << "accept a connection:" << client_addr.ToString();

    // spawn a corotine handle this connection
    //-> IOEvent::wait -> channel::read/write -> codec::Decode -> Router ->
    //Handler ->
    co_go[peer_fd, client_addr, this]() {
      handle_connection(peer_fd, client_addr);
    };
  } while (1);
}

void IOService::handle_connection(int client_socket, const IPEndPoint& peer) {
  lt::net::IOBuffer in_buf;

  base::LtEv::Event event = base::LtEv::READ;

  base::MessageLoop* io_loop = base::MessageLoop::Current();

  IPEndPoint local;
  CHECK(lt::net::socketutils::GetLocalEndpoint(client_socket, &local));

  TCPNoDelay(client_socket);
  KeepAlive(client_socket, true);

  auto fdev = base::FdEvent::Create(nullptr, client_socket, base::LtEv::READ);

  SocketChannelPtr channel = TcpChannel::Create(client_socket, local, peer);

  channel->SetFdEvent(fdev.get());

  auto codec = lt::net::CodecFactory::NewServerService(codec_, io_loop);
  CHECK(codec) << "protocal:[line] not be registed";

  codec->SetDelegate(this);
  codec->SetHandler(handler_);
  codec->SetAsFdEvHandler(false);
  codec->BindSocket(fdev, std::move(channel));
  codec->StartProtocolService();

  co::IOEvent ioev(fdev.get());
  do {
    auto res = ioev.Wait();
    if (res == co::IOEvent::Error ||
        res == co::IOEvent::Timeout) {
      codec->CloseService(true);
      break;
    }
    codec->HandleEvent(fdev.get(), fdev->FiredEvent());
  } while (!codec->IsClosed());
  LOG(INFO) << "close client socket connection";
  fdev.reset();
}

void IOService::OnCodecReady(const lt::net::RefCodecService& service) {
  LOG(INFO) << "codec service ready:" << service->Channel()->ChannelInfo();
}

void IOService::OnCodecClosed(const lt::net::RefCodecService& service) {
  LOG(INFO) << "codec service closed:" << service->Channel()->ChannelInfo();
}

}  // namespace coso
