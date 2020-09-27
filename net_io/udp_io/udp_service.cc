#include "udp_service.h"

#include <memory>
#include <net_io/socket_utils.h>
#include "net_io/base/sockaddr_storage.h"
#include <base/message_loop/message_loop.h>

namespace lt {
namespace net {

RefUDPService UDPService::Create(base::MessageLoop* io,
                                 const IPEndPoint& ep) {
  return RefUDPService(new UDPService(io, ep));
}

UDPService::UDPService(base::MessageLoop* io, const IPEndPoint& ep)
  : io_(io),
    endpoint_(ep) {
}

void UDPService::StartService() {
  if (!io_->IsInLoopThread()) {
    auto functor = std::bind(&UDPService::StartService, shared_from_this());
    io_->PostTask(FROM_HERE, functor);
    return;
  }
  int socket = socketutils::CreateNoneBlockUDP(endpoint_.GetSockAddrFamily());
  if (socket < 0) {
    return;
  }
  //reuse socket addr and port if possible
  socketutils::ReUseSocketPort(socket, true);
  socketutils::ReUseSocketAddress(socket, true);

  SockaddrStorage storage;
  endpoint_.ToSockAddr(storage.addr, storage.addr_len);

  int ret = socketutils::BindSocketFd(socket, storage.addr);
  if (ret < 0) {
    socketutils::CloseSocket(socket);
    return;
  }
  socket_event_ = base::FdEvent::Create(this, socket, base::LtEv::LT_EVENT_READ);
  io_->Pump()->InstallFdEvent(socket_event_.get());
}

void UDPService::StopService() {

}

//override from FdEvent::Handler
bool UDPService::HandleRead(base::FdEvent* fd_event) {
  LOG(INFO) << __FUNCTION__ << "fd_event:" << fd_event;
  return true;
}

bool UDPService::HandleWrite(base::FdEvent* fd_event) {
  LOG(INFO) << __FUNCTION__ << "fd_event:" << fd_event;
  return true;
}

bool UDPService::HandleError(base::FdEvent* fd_event) {
  LOG(INFO) << __FUNCTION__ << "fd_event:" << fd_event;
  return true;
}

bool UDPService::HandleClose(base::FdEvent* fd_event) {
  LOG(INFO) << __FUNCTION__ << "fd_event:" << fd_event;
  return true;
}

}}

