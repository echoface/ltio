#include "io_service.h"

#include "base/coroutine/co_runner.h"
#include "base/coroutine/io_event.h"
#include "base/utils/sys_error.h"
#include "net_io/socket_utils.h"

using lt::net::SockaddrStorage;
using namespace lt::net::socketutils;

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
  co::IOEvent ioev(socket, base::LtEv::LT_EVENT_READ);
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
    VLOG(GLOG_VTRACE) << "accept a connection:" << client_addr.ToString();

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

  do {
    base::LtEvent event = base::LtEv::LT_EVENT_READ;
    if (in_buf.CanReadSize()) {
      event |= base::LtEv::LT_EVENT_WRITE;
    }
    co::IOEvent(client_socket, (base::LtEv)event).Wait();

    // write data
    if (in_buf.CanReadSize()) {
      ssize_t writen =
          write(client_socket, in_buf.GetRead(), in_buf.CanReadSize());
      if (writen > 0) {
        in_buf.Consume(writen);
      } else if (errno != EAGAIN) {
        LOG(INFO) << "break connection loop";
        break;
      }
    }

    in_buf.EnsureWritableSize(1024);
    ssize_t n = ::Read(client_socket, in_buf.GetWrite(), in_buf.CanWriteSize());
    if (n > 0) {
      in_buf.Produce(n);
    } else if (n == 0) {
      LOG(INFO) << "break connection loop, peer close, eof";
      break;
    } else if (errno != EAGAIN) {
      LOG(INFO) << "break connection loop for error:" << base::StrError();
      break;
    }
  } while (1);
  LOG(INFO) << "close client socket connection";
  close(client_socket);
}

}  // namespace coso
