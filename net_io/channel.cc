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

//
// Created by gh on 18-12-5.
//
#include "channel.h"
#include "glog/logging.h"
#include "socket_utils.h"

#include <base/utils/sys_error.h>
#include "base/base_constants.h"
#include "base/message_loop/event_pump.h"

// socket chennel interface and base class
using base::FdEvent;

namespace lt {
namespace net {

SocketChannel::SocketChannel(int socket_fd,
                             const IPEndPoint& loc,
                             const IPEndPoint& peer)
  : local_ep_(loc),
    remote_ep_(peer) {

  fd_event_ = FdEvent::Create(this, socket_fd, base::LtEv::LT_EVENT_NONE);
}

SocketChannel::~SocketChannel() {
  CHECK(!IsConnected()) << ChannelInfo();
}

void SocketChannel::SetIOEventPump(base::EventPump* pump) {
  VLOG(GLOG_VTRACE) << "set pump:" << pump << ", id:" << pump->LoopID();
  pump_ = pump;
}

void SocketChannel::SetReciever(SocketChannel::Reciever* rec) {
  VLOG(GLOG_VTRACE) << "set reciever:" << rec << " fd:" << binded_fd();
  reciever_ = rec;
}

bool SocketChannel::StartChannel() {
  CHECK(reciever_ && pump_->IsInLoop() && status_ == Status::CONNECTING);

  setup_channel();

  VLOG(GLOG_VINFO) << __FUNCTION__ << " notify ready";
  reciever_->OnChannelReady(this);
  return true;
}

void SocketChannel::ShutdownChannel(bool half_close) {
  VLOG(GLOG_VTRACE) << __FUNCTION__ << ChannelInfo();
  DCHECK(pump_->IsInLoop());

  if (half_close) {
    schedule_shutdown_ = true;
    fd_event_->EnableWriting();
    return SetChannelStatus(Status::CLOSING);
  }

  if (!IsClosed()) {
    close_channel();
  }
  reciever_->OnChannelClosed(this);
}

void SocketChannel::ShutdownWithoutNotify() {
  DCHECK(pump_->IsInLoop());
  VLOG(GLOG_VTRACE) << __FUNCTION__ << ChannelInfo();
  if (!IsClosed()) {
    close_channel();
  }
}

bool SocketChannel::HandleClose(base::FdEvent* event) {
  VLOG(GLOG_VTRACE) << "close socket channel:" << ChannelInfo();
  if (!IsClosed()) {
    close_channel();
  }
  reciever_->OnChannelClosed(this);
  return false;  // disable next event
}

void SocketChannel::setup_channel() {
  VLOG(GLOG_VINFO) << __FUNCTION__ << " enter";
  fd_event_->EnableReading();
  // fd_event_->EnableWriting();
  pump_->InstallFdEvent(fd_event_.get());
  SetChannelStatus(Status::CONNECTED);
}

void SocketChannel::close_channel() {
  pump_->RemoveFdEvent(fd_event_.get());
  SetChannelStatus(Status::CLOSED);
}

int32_t SocketChannel::binded_fd() const {
  return fd_event_ ? fd_event_->GetFd() : -1;
}

std::string SocketChannel::local_name() const {
  return local_ep_.ToString();
}

std::string SocketChannel::remote_name() const {
  return remote_ep_.ToString();
}

std::string SocketChannel::ChannelInfo() const {
  std::ostringstream oss;
  oss << "[" << binded_fd() << "@<" << local_name() << "#" << remote_name()
      << ">" << StatusStr() << "]";
  return oss.str();
}

void SocketChannel::SetChannelStatus(Status st) {
  status_ = st;
}

const std::string SocketChannel::StatusStr() const {
  switch (status_) {
    case Status::CONNECTING:
      return "CONNECTING";
    case Status::CONNECTED:
      return "ESTABLISHED";
    case Status::CLOSING:
      return "CLOSING";
    case Status::CLOSED:
      return "CLOSED";
    default:
      break;
  }
  return "UNKNOWN";
}

}  // namespace net
}  // namespace lt
