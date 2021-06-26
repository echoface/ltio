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

#include "async_channel.h"
#include "queued_channel.h"
#include <base/base_constants.h>
#include <net_io/codec/codec_service.h>

namespace lt {
namespace net {

RefQueuedChannel QueuedChannel::Create(Delegate* d, const RefCodecService& s) {
  return RefQueuedChannel(new QueuedChannel(d, s));
}

QueuedChannel::QueuedChannel(Delegate* d, const RefCodecService& service)
  : ClientChannel(d, service) {
}

QueuedChannel::~QueuedChannel() {
}

void QueuedChannel::StartClientChannel() {
  ClientChannel::StartClientChannel();

  // Do other thing for initialize
}

void QueuedChannel::SendRequest(RefCodecMessage request)  {
  CHECK(IOLoop()->IsInLoopThread());

  request->SetIOCtx(codec_);
  waiting_list_.push_back(std::move(request));

  TrySendNext();
}

bool QueuedChannel::TrySendNext() {
  if (ing_request_) {
    return true;
  }

  bool success = false;
  while(!success && waiting_list_.size()) {

    RefCodecMessage& next = waiting_list_.front();
    success = codec_->SendRequest(next.get());
    waiting_list_.pop_front();
    if (success) {
      ing_request_ = next;
    } else {
      next->SetFailCode(MessageCode::kConnBroken);
      HandleResponse(next, CodecMessage::kNullMessage);
    }
  }

  if (success) {
    DCHECK(ing_request_);
    WeakCodecMessage weak(ing_request_);   // weak ptr must init outside, Take Care of weakptr
    auto functor = std::bind(&QueuedChannel::OnRequestTimeout, shared_from_this(), weak);
    IOLoop()->PostDelayTask(NewClosure(functor), request_timeout_);
  }
  return success;
}

void QueuedChannel::BeforeCloseChannel() {
  if (ing_request_) {
    ing_request_->SetFailCode(MessageCode::kConnBroken);
    HandleResponse(ing_request_, CodecMessage::kNullMessage);
    ing_request_.reset();
  }

  while(waiting_list_.size()) {
    RefCodecMessage& request = waiting_list_.front();
    request->SetFailCode(MessageCode::kConnBroken);
    HandleResponse(request, CodecMessage::kNullMessage);
    waiting_list_.pop_front();
  }
}

void QueuedChannel::OnRequestTimeout(WeakCodecMessage weak) {

  RefCodecMessage request = weak.lock();
  if (!request) return;

  if (request.get() != ing_request_.get()) {
    return;
  }

  VLOG(GLOG_VINFO) << __FUNCTION__ << codec_->Channel()->ChannelInfo() << " timeout reached";

  request->SetFailCode(MessageCode::kTimeOut);
  HandleResponse(request, CodecMessage::kNullMessage);
  ing_request_.reset();

  if (codec_->IsConnected()) {
  	codec_->CloseService();
  } else {
  	OnProtocolServiceGone(codec_);
  }
}

void QueuedChannel::OnCodecMessage(const RefCodecMessage& res) {
  DCHECK(IOLoop()->IsInLoopThread());

  if (!ing_request_) {
    return ;
  }

  const RefCodecMessage guard_req(ing_request_);
  ing_request_.reset();

  HandleResponse(guard_req, res);

  TrySendNext();
}

void QueuedChannel::OnProtocolServiceGone(const RefCodecService& service) {
  VLOG(GLOG_VTRACE) << __FUNCTION__ << service->Channel()->ChannelInfo() << " protocol service closed";
  if (ing_request_) {
    ing_request_->SetFailCode(MessageCode::kConnBroken);
    HandleResponse(ing_request_, CodecMessage::kNullMessage);
    ing_request_.reset();
  }

  while(waiting_list_.size()) {
    RefCodecMessage& request = waiting_list_.front();
    request->SetFailCode(MessageCode::kConnBroken);
    HandleResponse(request, CodecMessage::kNullMessage);
    waiting_list_.pop_front();
  }

  auto guard = shared_from_this();
  if (delegate_) {
    delegate_->OnClientChannelClosed(guard);
  }

  ClientChannel::OnProtocolServiceGone(service);
}

}}//net
