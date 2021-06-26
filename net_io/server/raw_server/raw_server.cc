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

#include "glog/logging.h"
#include "net_io/tcp_channel.h"
#include "net_io/url_utils.h"
#include "net_io/io_service.h"
#include "net_io/codec/codec_service.h"
#include "base/message_loop/linux_signal.h"
#include "net_io/codec/codec_factory.h"

#include <algorithm>
#include "raw_server.h"

namespace lt {
namespace net {

//static
RefRawRequestContext RawRequestContext::New(const RefCodecMessage& request) {
  return RefRawRequestContext(new RawRequestContext(request));
}

void RawRequestContext::Response(const RefCodecMessage& response) {
  if (did_reply_)
    return;

  return do_response(response);
}

void RawRequestContext::do_response(const RefCodecMessage& response) {
  did_reply_ = true;
  auto service = request_->GetIOCtx().codec.lock();
  if (!service) {
    LOG(ERROR) << __FUNCTION__ << " this connections has broken";
    return;
  }
  base::MessageLoop* io = service->IOLoop();

  if (io->IsInLoopThread()) {
    if (!service->SendResponse(request_.get(), response.get())) {
      service->CloseService();
    }
    return;
  }

  auto request = request_;
  auto functor = [service, request, response]() {
    if (!service->SendResponse(request.get(), response.get())) {
      service->CloseService();
    }
  };
  io->PostTask(NewClosure(std::move(functor)));
}

}  // namespace net
}  // namespace lt
