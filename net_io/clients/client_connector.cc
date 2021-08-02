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

#include <algorithm>

#include <base/utils/sys_error.h>
#include "base/message_loop/event.h"
#include "client_connector.h"
#include "net_io/base/sockaddr_storage.h"

namespace lt {
namespace net {

Connector::Connector(EventPump* pump) : pump_(pump) {
  count_.store(0);
}

void Connector::Dial(const net::IPEndPoint& ep, Connector::Delegate* r) {
  CHECK(pump_->IsInLoop() && r);

  int sock_fd = socketutils::CreateNoneBlockTCP(ep.GetSockAddrFamily());
  if (sock_fd == -1) {
    LOG(ERROR) << "connect [" << ep.ToString() << "] failed";
    return r->OnConnectFailed(inprogress_.size());
  }
  SockaddrStorage storage;
  if (ep.ToSockAddr(storage.AsSockAddr(), storage.Size()) == 0) {
    socketutils::CloseSocket(sock_fd);
    return r->OnConnectFailed(inprogress_.size());
  }

  int error = 0;
  int ret = socketutils::Connect(sock_fd, storage.AsSockAddr(), &error);
  if (ret == 0 && error == 0) {
    IPEndPoint remote_addr;
    IPEndPoint local_addr;

    if (!socketutils::GetPeerEndpoint(sock_fd, &remote_addr) ||
        !socketutils::GetLocalEndpoint(sock_fd, &local_addr)) {
      socketutils::CloseSocket(sock_fd);
      return r->OnConnectFailed(inprogress_.size());
    }

    VLOG(GLOG_VTRACE) << ep.ToString() << " connected at once";
    return r->OnConnected(sock_fd, local_addr, remote_addr);
  }

  switch (error) {
    case EINTR:
    case EISCONN:
    case EINPROGRESS: {
      RefFdEvent ev =
          FdEvent::Create(this, sock_fd, base::LtEv::LT_EVENT_WRITE);

      pump_->InstallFdEvent(ev.get());

      inprogress_.push_back(connect_ctx{ev, r});

      VLOG(GLOG_VTRACE) << "connect inprogress fd:" << sock_fd;

    } break;
    default: {
      socketutils::CloseSocket(sock_fd);
      LOG(ERROR) << "launch connection failed:" << base::StrError(error);
      return r->OnConnectFailed(inprogress_.size());

    } break;
  }
}

ConnDetail Connector::DialSync(const net::IPEndPoint& ep) {

  int fd = socketutils::CreateBlockTCPSocket(ep.GetSockAddrFamily());
  if (fd == -1) {
    return {};
  }

  SockaddrStorage storage;
  if (ep.ToSockAddr(storage.AsSockAddr(), storage.Size()) == 0) {
    socketutils::CloseSocket(fd);
    return {};
  }

  int error = 0;
  int ret = socketutils::Connect(fd, storage.AsSockAddr(), &error);
  if (ret < 0) {
    LOG(ERROR) << ep.ToString() << " connect fail" << base::StrError(error);
    return {};
  }

  ConnDetail detail;

  if (!socketutils::GetPeerEndpoint(fd, &detail.peer) ||
      !socketutils::GetLocalEndpoint(fd, &detail.local)) {
    socketutils::CloseSocket(fd);
    return {};
  }

  detail.socket = fd;
  socketutils::SetSocketBlocking(fd, true);

  VLOG(GLOG_VTRACE) << ep.ToString() << " connected success";
  return detail;
}


void Connector::HandleEvent(FdEvent* fdev) {
  if (fdev->RecvWrite()) {
    return HandleWrite(fdev);
  }
  //read/error/close
err_handle:
  HandleError(fdev);
}

void Connector::HandleError(base::FdEvent* fd_event) {
  LOG(ERROR) << "connect fd error" << base::StrError();
  RemoveFromInprogress(fd_event, true);
}

void Connector::HandleWrite(base::FdEvent* event) {
  int socket_fd = event->GetFd();
  if (socketutils::IsSelfConnect(socket_fd)) {
    LOG(ERROR) << "detect a self connection, endup";
    RemoveFromInprogress(event, true);
    return;
  }

  auto pred = [event](const connect_ctx& c) { return event == c.ev.get(); };
  auto iter = std::find_if(inprogress_.begin(), inprogress_.end(), pred);
  if (iter == inprogress_.end()) {
    LOG(ERROR) << "this connect request has been canceled";
    return;
  }

  event->ReleaseOwnership();
  pump_->RemoveFdEvent(event);

  connect_ctx c = *iter;
  inprogress_.erase(iter);

  IPEndPoint remote_addr, local_addr;
  if (!socketutils::GetPeerEndpoint(socket_fd, &remote_addr) ||
      !socketutils::GetLocalEndpoint(socket_fd, &local_addr)) {
    LOG(ERROR) << "bad socket fd, connect failed";
    c.hdl->OnConnectFailed(inprogress_.size());
    return;
  }
  c.hdl->OnConnected(socket_fd, local_addr, remote_addr);
}

void Connector::Stop() {
  CHECK(pump_->IsInLoop());

  for (auto& c : inprogress_) {
    pump_->RemoveFdEvent(c.ev.get());
  }
  inprogress_.clear();
  count_.store(inprogress_.size());
}

void Connector::RemoveFromInprogress(base::FdEvent* event, bool notify_fail) {
  DCHECK(pump_->IsInLoop());

  pump_->RemoveFdEvent(event);

  auto pred = [event](const connect_ctx& c) { return event == c.ev.get(); };
  auto iter = std::find_if(inprogress_.begin(), inprogress_.end(), pred);

  if (iter == inprogress_.end()) {
    LOG(ERROR) << "connect_ctx already removed:" << event->EventInfo();
    return;
  }

  if (notify_fail) {
    iter->hdl->OnConnectFailed(inprogress_.size());
  }
  inprogress_.erase(iter);
  count_.store(inprogress_.size());
}

}  // namespace net
}  // namespace lt
