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

#ifndef NET_RAW_SERVER_H_H
#define NET_RAW_SERVER_H_H

#include "base/message_loop/message_loop.h"
#include "net_io/codec/codec_message.h"
#include "net_io/codec/raw/raw_message.h"
#include "net_io/server/generic_server.h"

namespace lt {
namespace net {

class RawRequestContext;
using RefRawRequestContext = std::shared_ptr<RawRequestContext>;

class RawRequestContext {
public:
  static RefRawRequestContext New(const RefCodecMessage& request);

  template <typename T>
  const T* GetRequest() const {
    return (T*)request_.get();
  }

  void Response(const RefCodecMessage& response);

  bool Responded() const { return did_reply_; }

private:
  RawRequestContext(const RefCodecMessage& req) : request_(req){};

  void do_response(const RefCodecMessage& response);

  bool did_reply_ = false;

  RefCodecMessage request_;
};

struct DefaultRawNoneCoroConfigrator {
  static const bool coro_process = false;
  static const uint64_t kClientConnLimit = 65536;
  static const uint64_t kRequestQpsLimit = 100000;
};

using RawCoroServer = BaseServer<DefaultConfigurator>;

using RawServer = BaseServer<DefaultRawNoneCoroConfigrator>;

}  // namespace net
}  // namespace lt

#include "net_io/server/handler_factory.h"

template <typename Functor>
lt::net::CodecService::Handler* NewRawHandler(const Functor& func) {
  return NewHandler<Functor, lt::net::RawRequestContext>(func);
}

template <typename Functor>
lt::net::CodecService::Handler* NewRawCoroHandler(const Functor& func) {
  return NewCoroHandler<Functor, lt::net::RawRequestContext>(func);
}


#endif
