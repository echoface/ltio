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
#include <base/base_constants.h>
#include <net_io/codec/codec_service.h>
#include "base/message_loop/message_loop.h"

namespace lt {
namespace net {

RefAsyncChannel AsyncChannel::Create(Delegate* d, const RefCodecService& s) {
  return RefAsyncChannel(new AsyncChannel(d, s));
}

AsyncChannel::AsyncChannel(Delegate* d, const RefCodecService& s)
  : ClientChannel(d, s) {}

AsyncChannel::~AsyncChannel() {}

void AsyncChannel::StartClientChannel() {
  // common part
  ClientChannel::StartClientChannel();
}

void AsyncChannel::SendRequest(RefCodecMessage request) {
  CHECK(IOLoop()->IsInLoopThread());
  // guard herer ?
  // A -> b -> c -> call something it delete this Channel --> Back to A
  request->SetIOCtx(codec_);

  bool success = codec_->SendRequest(request.get());
  if (!success) {
    request->SetFailCode(MessageCode::kConnBroken);
    HandleResponse(request, CodecMessage::kNullMessage);
    return;
  }

  // weak ptr must init outside, Take Care of weakptr
  WeakCodecMessage weak(request);
  auto guard_this = shared_from_this();
  auto functor = std::bind(&AsyncChannel::OnRequestTimeout, guard_this, weak);

  IOLoop()->PostDelayTask(NewClosure(functor), request_timeout_);

  uint64_t message_identify = request->AsyncId();
  in_progress_.insert(std::make_pair(message_identify, std::move(request)));
}

void AsyncChannel::OnRequestTimeout(const WeakCodecMessage& weak) {
  DCHECK(IOLoop()->IsInLoopThread());

  auto request = weak.lock();
  if (!request) {
    return;
  }


  uint64_t identify = request->AsyncId();
  size_t numbers = in_progress_.erase(identify);
  if (numbers == 0) {
    VLOG(GLOG_VTRACE) << "message has reponsed";
    return;
  }
  request->SetFailCode(MessageCode::kTimeOut);
  HandleResponse(request, nullptr);
}

void AsyncChannel::BeforeCloseChannel() {
  DCHECK(IOLoop()->IsInLoopThread());

  for (auto kv : in_progress_) {
    kv.second->SetFailCode(MessageCode::kConnBroken);
    HandleResponse(kv.second, CodecMessage::kNullMessage);
  }
  in_progress_.clear();
}

void AsyncChannel::OnCodecMessage(const RefCodecMessage& res) {
  DCHECK(IOLoop()->IsInLoopThread());

  uint64_t identify = res->AsyncId();

  auto iter = in_progress_.find(identify);
  if (iter == in_progress_.end()) {
    VLOG(GLOG_VINFO) << __FUNCTION__ << " response:" << identify
                     << " not found corresponding request";
    return;
  }
  auto request = std::move(iter->second);
  in_progress_.erase(iter);
  CHECK(request->AsyncId() == identify);
  HandleResponse(request, res);
}

void AsyncChannel::OnCodecClosed(const RefCodecService& service) {
  VLOG(GLOG_VTRACE) << service->Channel()->ChannelInfo()
                    << " protocol service closed";
  DCHECK(IOLoop()->IsInLoopThread());
  auto guard = shared_from_this();

  for (auto kv : in_progress_) {
    kv.second->SetFailCode(MessageCode::kConnBroken);
    HandleResponse(kv.second, CodecMessage::kNullMessage);
  }
  in_progress_.clear();
  if (delegate_) {
    delegate_->OnClientChannelClosed(guard);
  }

  ClientChannel::OnCodecClosed(service);
}

}  // namespace net
}  // namespace lt
