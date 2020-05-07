#include <algorithm>

#include "client_connector.h"
#include <base/utils/sys_error.h>
#include "base/message_loop/event.h"
#include "net_io/base/sockaddr_storage.h"

namespace lt {
namespace net {

Connector::Connector(base::EventPump* pump, ConnectorDelegate* d)
  : pump_(pump),
    delegate_(d) {
  CHECK(delegate_);
}
bool Connector::Launch(const net::IPEndPoint &address) {
  CHECK(pump_->IsInLoopThread() && delegate_);

  int sock_fd = socketutils::CreateNonBlockingSocket(address.GetSockAddrFamily());
  if (sock_fd == -1) {
    LOG(ERROR) << __FUNCTION__ << " CreateNonBlockingSocket failed";
    return false;
  }

  VLOG(GLOG_VTRACE) << __FUNCTION__ << " connect to add:" << address.ToString();

  SockaddrStorage storage;
  if (!address.ToSockAddr(storage.addr, storage.addr_len)) {
    socketutils::CloseSocket(sock_fd);
    return false;
  }

  bool success = false;

  int error = 0;
  int ret = socketutils::SocketConnect(sock_fd, storage.addr, &error);
  VLOG(GLOG_VTRACE) << __FUNCTION__ << " ret:" << ret << " error:" << error;
  if (ret == 0 && error == 0) {
    IPEndPoint remote_addr;
    IPEndPoint local_addr;

    if (!socketutils::GetPeerEndpoint(sock_fd, &remote_addr) ||
        !socketutils::GetLocalEndpoint(sock_fd, &local_addr)) {
      socketutils::CloseSocket(sock_fd);
      return false;
    }
    delegate_->OnNewClientConnected(sock_fd, local_addr, remote_addr);
    VLOG(GLOG_VTRACE) << __FUNCTION__ << " " << address.ToString() << " connected at once";
    return true;
  }

  VLOG(GLOG_VINFO) << __FUNCTION__ << " launch connection err:" << base::StrError(error);
  switch(error) {
    case EINTR:
    case EISCONN:
    case EINPROGRESS: {
      success = true;
      base::RefFdEvent fd_event =
        base::FdEvent::Create(this, sock_fd, base::LtEv::LT_EVENT_WRITE);

      pump_->InstallFdEvent(fd_event.get());

      fd_event->EnableWriting();
      connecting_sockets_.insert(fd_event);
      VLOG(GLOG_VTRACE) << __FUNCTION__ << " " << address.ToString() << " connecting, fd:" << fd_event->fd();
    } break;
    default: {
      success = false;
      LOG(ERROR) << __FUNCTION__ << " launch connection failed:" << base::StrError(error);
    } break;
  }

  if (!success) {
    delegate_->OnClientConnectFailed();
    socketutils::CloseSocket(sock_fd);
  }
  return success;
}

void Connector::HandleRead(base::FdEvent* fd_event) {
  LOG(ERROR) << __func__ << " should not reached";
}

void Connector::HandleWrite(base::FdEvent* fd_event) {

  int socket_fd = fd_event->fd();
  if (!delegate_) {
    CleanUpBadChannel(fd_event);
    return;
  }

  if (net::socketutils::IsSelfConnect(socket_fd)) {
    CleanUpBadChannel(fd_event);
    LOG(ERROR) << __func__ << " IsSelfConnect, clean...";
    delegate_->OnClientConnectFailed();
    return;
  }

  fd_event->GiveupOwnerFd();
  pump_->RemoveFdEvent(fd_event);
  remove_fdevent(fd_event); //destructor here

  IPEndPoint remote_addr;
  IPEndPoint local_addr;
  if (!socketutils::GetPeerEndpoint(socket_fd, &remote_addr) ||
      !socketutils::GetLocalEndpoint(socket_fd, &local_addr)) {
    CleanUpBadChannel(fd_event);
    LOG(ERROR) << __func__ << " bad socket fd, clean...";
    delegate_->OnClientConnectFailed();
    return;
  }
  delegate_->OnNewClientConnected(socket_fd, local_addr, remote_addr);
}

void Connector::HandleError(base::FdEvent* fd_event) {
  CleanUpBadChannel(fd_event);
  if (delegate_) {
    delegate_->OnClientConnectFailed();
  }
}

void Connector::HandleClose(base::FdEvent* fd_event) {
  HandleError(fd_event);
}

void Connector::Stop() {
  CHECK(pump_->IsInLoopThread());

  delegate_ = NULL;
  for (auto& event : connecting_sockets_) {
    event->GiveupOwnerFd();
    pump_->RemoveFdEvent(event.get());

    int socket_fd = event->fd();
    net::socketutils::CloseSocket(socket_fd);
  }
  connecting_sockets_.clear();
}

void Connector::CleanUpBadChannel(base::FdEvent* event) {

  event->ResetCallback();
  pump_->RemoveFdEvent(event);

  remove_fdevent(event); //destructor here
  VLOG(GLOG_VINFO) << " connecting list size:" << connecting_sockets_.size();
}

bool Connector::remove_fdevent(base::FdEvent* event) {
  auto iter = connecting_sockets_.begin();
  for (; iter != connecting_sockets_.end(); iter++) {
    if (iter->get() == event) {
      iter = connecting_sockets_.erase(iter);
      return true;
    }
  }
  return false;
}


}}//end namespace
