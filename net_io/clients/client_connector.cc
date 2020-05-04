#include <algorithm>

#include "client_connector.h"
#include <base/utils/sys_error.h>
#include "base/message_loop/event.h"

namespace lt {
namespace net {

Connector::Connector(base::EventPump* pump, ConnectorDelegate* d)
  : pump_(pump),
    delegate_(d) {
  CHECK(delegate_);
}

bool Connector::Launch(const net::SocketAddr &address) {
  CHECK(pump_->IsInLoopThread());

  int sock_fd = net::socketutils::CreateNonBlockingSocket(address.Family());
  if (sock_fd == -1) {
    LOG(ERROR) << __FUNCTION__ << " CreateNonBlockingSocket failed";
    return false;
  }

  VLOG(GLOG_VTRACE) << __FUNCTION__ << " connect to add:" << address.IpPort();
  const struct sockaddr* sock_addr = net::socketutils::sockaddr_cast(address.SockAddrIn());

  bool success = false;

  int error = 0;
  int ret = socketutils::SocketConnect(sock_fd, sock_addr, &error);
  VLOG(GLOG_VTRACE) << __FUNCTION__ << " ret:" << ret << " error:" << error;
  if (ret == 0 && error == 0) {
    net::SocketAddr remote_addr(net::socketutils::GetPeerAddrIn(sock_fd));
    net::SocketAddr local_addr(net::socketutils::GetLocalAddrIn(sock_fd));
    if (delegate_) {
      delegate_->OnNewClientConnected(sock_fd, local_addr, remote_addr);
    } else {
      net::socketutils::CloseSocket(sock_fd);
    }
    VLOG(GLOG_VTRACE) << __FUNCTION__ << " " << address.IpPort() << " connected at once";
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
      VLOG(GLOG_VTRACE) << __FUNCTION__ << " " << address.IpPort() << " connecting, fd:" << fd_event->fd();
    } break;
    default: {
      success = false;
      LOG(ERROR) << __FUNCTION__ << " launch connection failed:" << base::StrError(error);
    } break;
  }

  if (!success) {
    if (delegate_) {
      delegate_->OnClientConnectFailed();
    }
    net::socketutils::CloseSocket(sock_fd);
  }

  return success;
}

void Connector::HandleRead(base::FdEvent* fd_event) {
  LOG(ERROR) << __func__ << " should not reached";
}

void Connector::HandleWrite(base::FdEvent* fd_event) {
  //base::RefFdEvent fd_event = weak_fdevent.lock();
  int socket_fd = fd_event->fd();

  if (net::socketutils::IsSelfConnect(socket_fd)) {

    CleanUpBadChannel(fd_event);
    delegate_->OnClientConnectFailed();
    LOG(ERROR) << __func__ << " IsSelfConnect, clean...";

    return;
  }

  pump_->RemoveFdEvent(fd_event);
  fd_event->GiveupOwnerFd();

  remove_fdevent(fd_event); //destructor here

  net::SocketAddr remote_addr(net::socketutils::GetPeerAddrIn(socket_fd));
  net::SocketAddr local_addr(net::socketutils::GetLocalAddrIn(socket_fd));
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
