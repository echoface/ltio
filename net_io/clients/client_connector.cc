#include "client_connector.h"
#include "message_loop/event.h"
#include <base/utils/sys_error.h>

namespace net {

Connector::Connector(base::MessageLoop* loop, ConnectorDelegate* delegate)
  : loop_(loop),
    delegate_(delegate) {
  CHECK(delegate_);
}

bool Connector::Launch(const net::SocketAddress &address) {
  CHECK(loop_->IsInLoopThread());

  int sock_fd = net::socketutils::CreateNonBlockingSocket(address.Family());
  if (sock_fd == -1) {
    LOG(INFO) << __FUNCTION__ << " create none block socket failed";
    return false;
  }

  const struct sockaddr* sock_addr = net::socketutils::sockaddr_cast(address.SockAddrIn());

  bool success = false;

  int state = 0;
  net::socketutils::SocketConnect(sock_fd, sock_addr, &state);

  switch(state)  {
    case 0:
    case EINTR:
    case EISCONN:
    case EINPROGRESS: {

      success = true;
      base::RefFdEvent fd_event = base::FdEvent::Create(sock_fd, base::LtEv::LT_EVENT_NONE);

      WeakPtrFdEvent weak_fd_event(fd_event);
      fd_event->SetWriteCallback(std::bind(&Connector::OnWrite, this, weak_fd_event));
      fd_event->SetErrorCallback(std::bind(&Connector::OnError, this, weak_fd_event));
      fd_event->SetCloseCallback(std::bind(&Connector::OnError, this, weak_fd_event));
      InitEvent(fd_event);

      connecting_sockets_.insert(fd_event);
    } break;
    default: {
      success = false;
      LOG(ERROR) << " setup client connect failed:" << base::StrError(state);
      net::socketutils::CloseSocket(sock_fd);
    } break;
  }
  if (!success) {
    delegate_->OnClientConnectFailed();
  }
  return success;
}

void Connector::OnWrite(WeakPtrFdEvent weak_fdevent) {
  base::RefFdEvent fd_event = weak_fdevent.lock();

  if (!fd_event) {
    LOG(ERROR) << __FUNCTION__ << " FdEvent Has Gone Before Connection Setup";
    delegate_->OnClientConnectFailed();
    return ;
  }

  int socket_fd = fd_event->fd();

  if (net::socketutils::IsSelfConnect(socket_fd)) {
    CleanUpBadChannel(fd_event);
    delegate_->OnClientConnectFailed();
    return;
  }

  loop_->Pump()->RemoveFdEvent(fd_event.get());

  fd_event->GiveupOwnerFd();
  connecting_sockets_.erase(fd_event);

  net::SocketAddress remote_addr(net::socketutils::GetPeerAddrIn(socket_fd));
  net::SocketAddress local_addr(net::socketutils::GetLocalAddrIn(socket_fd));

  delegate_->OnNewClientConnected(socket_fd, local_addr, remote_addr);
}

void Connector::OnError(WeakPtrFdEvent weak_fdevent) {
  base::RefFdEvent event = weak_fdevent.lock();
  if (!event) {
    delegate_->OnClientConnectFailed();
    return;
  }

  CleanUpBadChannel(event);
  delegate_->OnClientConnectFailed();
}

void Connector::DiscardAllConnectingClient() {
  CHECK(loop_->IsInLoopThread());
  for (base::RefFdEvent event : connecting_sockets_) {
    int socket_fd = event->fd();
    loop_->Pump()->RemoveFdEvent(event.get());
    net::socketutils::CloseSocket(socket_fd);
  }
  connecting_sockets_.clear();
}

void Connector::InitEvent(base::RefFdEvent& fd_event) {
  loop_->Pump()->InstallFdEvent(fd_event.get());
  fd_event->EnableWriting();
}

void Connector::CleanUpBadChannel(base::RefFdEvent& event) {

  event->ResetCallback();
  loop_->Pump()->RemoveFdEvent(event.get());

  connecting_sockets_.erase(event);
  LOG(INFO) << "Now Has " << connecting_sockets_.size() << " In Connecting Progress";
}

}//end namespace