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

#include "glog/logging.h"

namespace lt {
namespace net {

CodecService::CodecService(base::MessageLoop* loop) : loop_(loop) {}

CodecService::~CodecService() {
  VLOG(GLOG_VTRACE) << __func__ << " this@" << this << " gone";
};

void CodecService::BindSocket(base::RefFdEvent&& fdev,
                              SocketChannelPtr&& channel) {
  DCHECK(!channel_.get());
  fdev_ = std::move(fdev);
  channel_ = std::move(channel);

  channel_->SetFdEvent(fdev_.get());
}

void CodecService::StartProtocolService() {
  VLOG(GLOG_VINFO) << __FUNCTION__ << " enter";

  StartInternal();

  // ready for default, if a codec has init action, need
  // override StartProtocolService and do init and then notify
  NotifyCodecReady();
}

void CodecService::StartInternal() {
  CHECK(loop_->IsInLoopThread());

  if (as_fdev_handler_) {
    fdev_->SetHandler(this);
    Pump()->InstallFdEvent(fdev_.get());
  }

  ignore_result(channel_->StartChannel(IsServerSide()));
}

void CodecService::CloseService(bool block_callback) {
  DCHECK(loop_->IsInLoopThread());

  BeforeCloseService();

  if (as_fdev_handler_) {
    CHECK(fdev_->GetHandler() == this);
    Pump()->RemoveFdEvent(fdev_.get());
  }
}

void CodecService::SetIsServerSide(bool server_side) {
  server_side_ = server_side;
}

void CodecService::HandleEvent(base::FdEvent* fdev) {
  if (channel_->HasOutgoingData()) {
    if (!channel_->TryFlush()) {
      goto err_handle;
    }
    if (!channel_->HasOutgoingData()) {
      OnDataFinishSend();
    }
  }

  if (fdev->ReadFired()) {
    if (!channel_->HandleRead()) {
      goto err_handle;
    }
  }
  if (channel_->HasIncommingData()) {
    OnDataReceived(channel_->ReaderBuffer());
  }
  if (!schedule_close_) {
    return;
  }
err_handle:
  CloseService(true);
  return NotifyCodecClosed();
}

void CodecService::NotifyCodecClosed() {
  VLOG(GLOG_VTRACE) << channel_->ChannelInfo() << " closed";
  AfterChannelClosed();
  if (delegate_) {
    delegate_->OnCodecClosed(shared_from_this());
  }
}

void CodecService::NotifyCodecReady() {
  VLOG(GLOG_VINFO) << __FUNCTION__ << " enter";
  if (delegate_) {
    delegate_->OnCodecReady(shared_from_this());
  }
}

}  // namespace net
}  // namespace lt
