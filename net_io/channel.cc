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
#include "base/logging.h"
#include "base/message_loop/event_pump.h"

// socket chennel interface and base class

namespace lt {
namespace net {

SocketChannel::SocketChannel(int socket,
                             const IPEndPoint& loc,
                             const IPEndPoint& peer)
  : local_ep_(loc),
    remote_ep_(peer) {}

SocketChannel::~SocketChannel() {}

bool SocketChannel::StartChannel(bool server) {

  fdev_->EnableWriting();
  fdev_->EnableReading();
  fdev_->SetEdgeTrigger(true);
  return true;
};

std::string SocketChannel::local_name() const {
  return local_ep_.ToString();
}

std::string SocketChannel::remote_name() const {
  return remote_ep_.ToString();
}

std::string SocketChannel::ChannelInfo() const {
  std::ostringstream oss;
  oss << "[" << binded_fd() << "@<" << local_name() << "#" << remote_name()
      << ">]";
  return oss.str();
}

}  // namespace net
}  // namespace lt
