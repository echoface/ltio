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

#include "codec_service.h"
#include "codec_message.h"

#include <sys/mman.h>
#include <unistd.h>

#include "base/message_loop/message_loop.h"
#include "glog/logging.h"
#include "net_io/channel.h"

namespace lt {
namespace net {

CodecService::CodecService(base::MessageLoop* loop) : loop_(loop) {}

CodecService::~CodecService() {
  VLOG(GLOG_VTRACE) << __func__ << " this@" << this << " gone";
};

void CodecService::BindSocket(SocketChannelPtr&& channel) {
  DCHECK(!channel_.get());
  channel_ = std::move(channel);
  channel_->SetReciever(this);
}

base::EventPump* CodecService::Pump() const {
  return loop_->Pump();;
}

bool CodecService::IsConnected() const {
  return channel_ ? channel_->IsConnected() : false;
}

void CodecService::StartProtocolService() {
  VLOG(GLOG_VINFO) << __FUNCTION__ << " enter";
  CHECK(loop_->IsInLoopThread());

  ignore_result(channel_->StartChannel());
}

void CodecService::CloseService(bool block_callback) {
  DCHECK(loop_->IsInLoopThread());

  BeforeCloseService();

  if (block_callback) {
    return channel_->ShutdownWithoutNotify();
  }
  return channel_->ShutdownChannel(false);
}

void CodecService::SetIsServerSide(bool server_side) {
  server_side_ = server_side;
}

void CodecService::OnChannelClosed(const SocketChannel* channel) {
  CHECK(channel == channel_.get());

  VLOG(GLOG_VTRACE) << channel_->ChannelInfo() << " closed";
  RefCodecService guard = shared_from_this();

  AfterChannelClosed();

  delegate_->OnCodecClosed(guard);
}

void CodecService::OnChannelReady(const SocketChannel*) {
  RefCodecService guard = shared_from_this();
  VLOG(GLOG_VINFO) << __FUNCTION__ << " enter";
  delegate_->OnCodecReady(guard);
}

}  // namespace net
}  // namespace lt
