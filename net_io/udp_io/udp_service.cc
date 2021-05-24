/*
 * Copyright 2021 <name of copyright holder>
 * Author: Huan.Gong <gonghuan.dev@gmail.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

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
    endpoint_(ep),
    buffers_(100) {
}

void UDPService::StartService() {
  if (!io_->IsInLoopThread()) {
    auto functor = std::bind(&UDPService::StartService, shared_from_this());
    io_->PostTask(FROM_HERE, functor);
    return;
  }

  int socket = socketutils::CreateNoneBlockUDP(endpoint_.GetSockAddrFamily());
  if (socket < 0) {
    LOG(ERROR) << "create socket failed, ep:" << endpoint_.ToString();
    return;
  }
  //reuse socket addr and port if possible
  socketutils::ReUseSocketPort(socket, true);
  socketutils::ReUseSocketAddress(socket, true);

  SockaddrStorage storage;
  endpoint_.ToSockAddr(storage.AsSockAddr(), storage.Size());

  int ret = socketutils::BindSocketFd(socket, storage.AsSockAddr());
  if (ret < 0) {
    LOG(ERROR) << "bind socket failed, socket:" << socket << " ep:" << endpoint_.ToString();
    socketutils::CloseSocket(socket);
    return;
  }
  socket_event_ = base::FdEvent::Create(this, socket, base::LtEv::LT_EVENT_READ);
  io_->Pump()->InstallFdEvent(socket_event_.get());
}

void UDPService::StopService() {
  if (!io_->IsInLoopThread()) {
    io_->PostTask(FROM_HERE, &UDPService::StopService, shared_from_this());
    return;
  }
  if (!socket_event_) return;
  socketutils::CloseSocket(socket_event_->fd());
}

//override from FdEvent::Handler
bool UDPService::HandleRead(base::FdEvent* fd_event) {

  do {
    struct mmsghdr* hdr = buffers_.GetMmsghdr();
    int recv_cnt = recvmmsg(fd_event->fd(), hdr, buffers_.Count(), MSG_WAITFORONE, NULL);
    if (recv_cnt <= 0) {
      if (errno == EAGAIN || errno == EINTR) {
        break;
      }

      return HandleClose(fd_event);
    }

    for (int i = 0; i < recv_cnt; i++) {

      UDPBufferDetail detail = buffers_.GetBufferDetail(i);
      IPEndPoint endpoint;
      bool ok = endpoint.FromSockAddr(detail.buffer->src_addr_.AsSockAddr(), detail.buffer->src_addr_.Size());
      LOG_IF(ERROR, !ok) << "send addr can't be parse, check implement, should be a bug";
      LOG(INFO) << "recv:" << detail.data() << " sender:" << endpoint.ToString();
    }
  } while(true);

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

