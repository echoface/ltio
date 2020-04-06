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

  switch(error) {
    case EINTR:
    case EISCONN:
    case EINPROGRESS: {
      success = true;
      base::RefFdEvent fd_event = base::FdEvent::Create(sock_fd, base::LtEv::LT_EVENT_NONE);

      WeakPtrFdEvent weak_fd_event(fd_event);
      fd_event->SetWriteCallback(std::bind(&Connector::OnWrite, this, weak_fd_event));
      fd_event->SetErrorCallback(std::bind(&Connector::OnError, this, weak_fd_event));
      fd_event->SetCloseCallback(std::bind(&Connector::OnError, this, weak_fd_event));

      fd_event->EnableWriting();
      pump_->InstallFdEvent(fd_event.get());
      connecting_sockets_.insert(fd_event);

      VLOG(GLOG_VTRACE) << __FUNCTION__ << " " << address.IpPort() << " connecting";
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

void Connector::OnWrite(WeakPtrFdEvent weak_fdevent) {
  base::RefFdEvent fd_event = weak_fdevent.lock();

  if (!fd_event && delegate_) {
    delegate_->OnClientConnectFailed();
    LOG(ERROR) << __FUNCTION__ << " FdEvent Has Gone Before Connection Setup";
    return;
  }

  int socket_fd = fd_event->fd();

  if (net::socketutils::IsSelfConnect(socket_fd)) {
    CleanUpBadChannel(fd_event);
    if (delegate_) {
      delegate_->OnClientConnectFailed();
    }
    return;
  }

  pump_->RemoveFdEvent(fd_event.get());
  connecting_sockets_.erase(fd_event);
  if (delegate_) {
    fd_event->GiveupOwnerFd();
    net::SocketAddr remote_addr(net::socketutils::GetPeerAddrIn(socket_fd));
    net::SocketAddr local_addr(net::socketutils::GetLocalAddrIn(socket_fd));
    delegate_->OnNewClientConnected(socket_fd, local_addr, remote_addr);
  }
}

void Connector::OnError(WeakPtrFdEvent weak_fdevent) {
  base::RefFdEvent event = weak_fdevent.lock();
  if (!event && delegate_) {
    delegate_->OnClientConnectFailed();
    return;
  }

  CleanUpBadChannel(event);
  if (delegate_) {
    delegate_->OnClientConnectFailed();
  }
}

void Connector::Stop() {
  CHECK(pump_->IsInLoopThread());

  delegate_ = NULL;
  for (auto& event : connecting_sockets_) {
    int socket_fd = event->fd();
    pump_->RemoveFdEvent(event.get());
    net::socketutils::CloseSocket(socket_fd);
  }
  connecting_sockets_.clear();
}

void Connector::CleanUpBadChannel(base::RefFdEvent& event) {

  event->ResetCallback();
  pump_->RemoveFdEvent(event.get());

  connecting_sockets_.erase(event);
  VLOG(GLOG_VINFO) << " connecting list size:" << connecting_sockets_.size();
}

}}//end namespace
