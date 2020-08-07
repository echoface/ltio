

#include "base/ip_endpoint.h"
#include "glog/logging.h"
#include "socket_acceptor.h"
#include "base/base_constants.h"
#include "base/utils/sys_error.h"
#include "base/sockaddr_storage.h"

namespace lt {
namespace net {

SocketAcceptor::SocketAcceptor(base::EventPump* pump, const IPEndPoint& address)
  : listening_(false),
    address_(address),
    event_pump_(pump) {
  CHECK(event_pump_);
  CHECK(InitListener());
}

SocketAcceptor::~SocketAcceptor() {
  CHECK(listening_ == false);
  socket_event_.reset();
}

bool SocketAcceptor::InitListener() {
  int socket_fd = socketutils::CreateNonBlockingSocket(address_.GetSockAddrFamily());
  if (socket_fd < 0) {
  	LOG(ERROR) << " failed create none blocking socket fd";
    return false;
  }
  //reuse socket addr and port if possible
  socketutils::ReUseSocketPort(socket_fd, true);
  socketutils::ReUseSocketAddress(socket_fd, true);

  SockaddrStorage storage;
  address_.ToSockAddr(storage.addr, storage.addr_len);

  int ret = socketutils::BindSocketFd(socket_fd, storage.addr);
  if (ret < 0) {
    LOG(ERROR) << " failed bind address for blocking socket fd";
    socketutils::CloseSocket(socket_fd);
    return false;
  }
  socket_event_ = base::FdEvent::Create(this, socket_fd, base::LtEv::LT_EVENT_NONE);

  VLOG(GLOG_VTRACE) << __FUNCTION__
    << " init acceptor success, fd:[" << socket_fd
    << "] bind to local:[" << address_.ToString() << "]";
  return true;
}

bool SocketAcceptor::StartListen() {
  CHECK(event_pump_->IsInLoopThread());

  if (listening_) {
    LOG(ERROR) << " Aready Listen on:" << address_.ToString();
    return true;
  }

  event_pump_->InstallFdEvent(socket_event_.get());
  socket_event_->EnableReading();

  bool success = socketutils::ListenSocket(socket_event_->fd()) == 0;
  if (!success) {
    socket_event_->DisableAll();
    event_pump_->RemoveFdEvent(socket_event_.get());
    LOG(INFO) << __FUNCTION__ << " failed listen on" << address_.ToString();
    return false;
  }
  VLOG(GLOG_VINFO) << "start listen on:" << address_.ToString();
  listening_ = true;
  return true;
}

void SocketAcceptor::StopListen() {
  CHECK(event_pump_->IsInLoopThread());
  if (!listening_) {
    return;
  }
  event_pump_->RemoveFdEvent(socket_event_.get());
  socket_event_->DisableAll();

  listening_ = false;
  VLOG(GLOG_VINFO) << "stop listen on:" << address_.ToString();
}

void SocketAcceptor::SetNewConnectionCallback(const NewConnectionCallback& cb) {
  new_conn_callback_ = std::move(cb);
}

bool SocketAcceptor::HandleRead(base::FdEvent* fd_event) {

  struct sockaddr client_socket_in;

  int err = 0;
  int peer_fd = socketutils::AcceptSocket(socket_event_->fd(), &client_socket_in, &err);
  if (peer_fd < 0) {
    LOG(ERROR) << __FUNCTION__ << " accept new connection failed, err:" << base::StrError(err);
    return true;
  }

  IPEndPoint client_addr;
  client_addr.FromSockAddr(&client_socket_in, sizeof(client_socket_in));

  VLOG(GLOG_VTRACE) << __FUNCTION__ << " accept a connection:" << client_addr.ToString();
  if (new_conn_callback_) {
    new_conn_callback_(peer_fd, client_addr);
    return true;
  }
  socketutils::CloseSocket(peer_fd);
  return true;
}

bool SocketAcceptor::HandleWrite(base::FdEvent* ev) {
  LOG(ERROR) << __FUNCTION__ << " fd [" << ev->fd() << "] write event should not reached";
  return true;
}

bool SocketAcceptor::HandleError(base::FdEvent* fd_event) {
  LOG(ERROR) << __FUNCTION__
    << " fd [" << fd_event->fd() << "] error:[" << base::StrError() << "]";

  listening_ = false;

  // Relaunch This server
  if (InitListener()) {
    bool re_listen_ok = StartListen();
    LOG_IF(ERROR, !re_listen_ok) << __FUNCTION__
      << " acceptor:[" << address_.ToString() << "] re-listen failed";
  }
  return false;
}

bool SocketAcceptor::HandleClose(base::FdEvent* fd_event) {
  LOG(ERROR) << __FUNCTION__ << " fd [" << fd_event->fd() << "] closed";
  return HandleError(fd_event);
}

}}//end namespace net
