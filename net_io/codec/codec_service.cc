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

#include <sys/mman.h>
#include <unistd.h>

#include "base/message_loop/message_loop.h"
#include "net_io/channel.h"
#include "net_io/socket_utils.h"

namespace lt {
namespace net {

CodecService::CodecService(base::MessageLoop* loop) : loop_(loop) {}

CodecService::~CodecService() {
  VLOG(VTRACE) << __func__ << " this@" << this << " gone";
};

void CodecService::BindSocket(base::RefFdEvent fdev,
                              SocketChannelPtr&& channel) {
  DCHECK(!channel_.get());
  fdev_ = std::move(fdev);
  channel_ = std::move(channel);

  channel_->SetFdEvent(fdev_.get());
}

void CodecService::StartProtocolService() {
  VLOG(VINFO) << __FUNCTION__ << " enter";

  StartInternal();

  status_ = Status::CONNECTED;

  // ready for default, if a codec has init action, need
  // override StartProtocolService and do init and then notify
  NotifyCodecReady();
}

void CodecService::StartInternal() {
  CHECK(loop_->IsInLoopThread());

  ignore_result(channel_->StartChannel(IsServerSide()));

  if (as_fdev_handler_) {
    fdev_->SetHandler(this);
    Pump()->InstallFdEvent(fdev_.get());
  }
}

void CodecService::CloseService(bool block_callback) {
  DCHECK(loop_->IsInLoopThread());
  VLOG(VTRACE) << __FUNCTION__ << ", enter";

  BeforeCloseService();

  if (as_fdev_handler_) {
    CHECK(fdev_->GetHandler() == this);
    Pump()->RemoveFdEvent(fdev_.get());
  }
  status_ = Status::CLOSED;
  if (!block_callback) {
    NotifyCodecClosed();
  }
}

void CodecService::SetIsServerSide(bool server_side) {
  server_side_ = server_side;
}

void CodecService::OnDataFinishSend() {
  if (!schedule_close_) {
    return;
  }
  return CloseService(false);
}

bool CodecService::ShouldClose() const {
  return schedule_close_ && (!channel_->HasOutgoingData());
}

bool CodecService::TryFlushChannel() {

  int nbytes = channel_->HandleWrite();

  if (nbytes > 0 && !channel_->HasOutgoingData()) {
    OnDataFinishSend();
  }
  return nbytes >= 0;
}

// handle out data and read data into buffer
bool CodecService::SocketReadWrite(base::LtEv::Event ev) {

  if (base::LtEv::has_write(ev)) {
    int rv = channel_->HandleWrite();
    if (rv < 0) {
      return false;
    }
    if (rv > 0 && !channel_->HasOutgoingData()) {
      OnDataFinishSend();
    }
  }

  if (base::LtEv::has_read(ev) && channel_->HandleRead() < 0) {
    return false;
  }

  return true;
}

void CodecService::HandleEvent(base::FdEvent* fdev, base::LtEv::Event ev) {
  VLOG(VTRACE) << __FUNCTION__ << ", enter";

  bool success = SocketReadWrite(ev);
  // r/w fatal error
  if (channel_->HasIncommingData()) {
    OnDataReceived(channel_->ReaderBuffer());
  }

  if (success && !ShouldClose() && !base::LtEv::has_error(ev)) {
    return;
  }

  return CloseService(false);
}

void CodecService::NotifyCodecClosed() {
  VLOG(VTRACE) << channel_->ChannelInfo() << " closed";
  AfterChannelClosed();
  if (delegate_) {
    delegate_->OnCodecClosed(shared_from_this());
  }
}

void CodecService::NotifyCodecReady() {
  VLOG(VINFO) << __FUNCTION__ << " enter";
  if (status_ == Status::CONNECTING) {
    status_ = Status::CONNECTED;
  }
  if (delegate_) {
    delegate_->OnCodecReady(shared_from_this());
  }
}

}  // namespace net
}  // namespace lt
