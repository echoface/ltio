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


#include "request_context.h"

namespace lt {
namespace net {

CommonRequestContext::CommonRequestContext(const RefCodecMessage&& req)
  : request_(req) {
};

CommonRequestContext::~CommonRequestContext() {
  if (!IsResponded()) {
    auto service = request_->GetIOCtx().codec.lock();
    if (!service) {
      return;
    }
    auto response = service->NewResponse(request_.get());
    do_response(response);
  }
}

void CommonRequestContext::Send(const RefCodecMessage& response) {
  if (done_) return;
  return do_response(response);
}

void CommonRequestContext::do_response(const RefCodecMessage& response) {
  done_ = true;
  auto service = request_->GetIOCtx().codec.lock();
  LOG_IF(ERROR, !service) << __FUNCTION__ << " connection has broken";
  if (!service) { return;}

  if (service->IOLoop()->IsInLoopThread()) {
    if (!service->EncodeResponseToChannel(request_.get(), response.get())) {
      service->CloseService();
    }
    return;
  }
  service->IOLoop()->PostTask(FROM_HERE, [=]() {
    if (!service->EncodeResponseToChannel(request_.get(), response.get())) {
      service->CloseService();
    }
  });
}

}}
