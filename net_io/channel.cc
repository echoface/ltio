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
#include "socket_utils.h"
#include "base/base_constants.h"
#include "base/message_loop/event_pump.h"
#include "glog/logging.h"

// socket chennel interface and base class
using base::FdEvent;

namespace lt {
namespace net {

SocketChannel::SocketChannel(int socket_fd,
                             const IPEndPoint& loc,
                             const IPEndPoint& peer,
                             base::EventPump* pump)
  : pump_(pump),
    schedule_shutdown_(false),
    local_ep_(loc),
    remote_ep_(peer),
    name_(loc.ToString()) {

    fd_event_ = FdEvent::Create(this,
                                socket_fd,
                                base::LtEv::LT_EVENT_NONE);
}

void SocketChannel::SetReciever(SocketChannel::Reciever* rec) {
  VLOG(GLOG_VTRACE) << "set reciever:" << rec << " fd:" << binded_fd();
  reciever_ = rec;
}

void SocketChannel::StartChannel() {
  CHECK(fd_event_ &&
        reciever_ &&
        pump_->IsInLoopThread() &&
        status_ == Status::CONNECTING);
  setup_channel();
}

bool SocketChannel::TryFlush() {
  HandleWrite(fd_event_.get());
  return true;
}

void SocketChannel::setup_channel() {

  fd_event_->EnableReading();
  pump_->InstallFdEvent(fd_event_.get());

  SetChannelStatus(Status::CONNECTED);

  reciever_->OnChannelReady(this);
}

void SocketChannel::close_channel() {
  pump_->RemoveFdEvent(fd_event_.get());
  SetChannelStatus(Status::CLOSED);
}

int32_t SocketChannel::binded_fd() const {
  return fd_event_ ? fd_event_->fd() : -1;
}

std::string SocketChannel::local_name() const {
  return local_ep_.ToString();
}

std::string SocketChannel::remote_name() const {
  return remote_ep_.ToString();
}

std::string SocketChannel::ChannelInfo() const {
  std::ostringstream oss;
  oss << "[fd:" << binded_fd()
    << ", this:" << this
    << ", local:" << local_name()
    << ", remote:" << remote_name()
    << ", status:" << StatusAsString() << "]";
  return oss.str();
}


void SocketChannel::SetChannelStatus(Status st) {
  status_ = st;
}

const std::string SocketChannel::StatusAsString() const {
  switch(status_) {
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

}}
