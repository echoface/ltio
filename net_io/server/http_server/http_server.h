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

#ifndef NET_HTTP_SERVER_H_H
#define NET_HTTP_SERVER_H_H

#include <list>
#include <vector>
#include <chrono>             // std::chrono::seconds
#include <functional>

#include "http_context.h"
#include "base/base_micro.h"
#include "base/message_loop/message_loop.h"
#include "net_io/io_service.h"
#include "net_io/codec/codec_message.h"
#include "net_io/codec/http/http_request.h"
#include "net_io/codec/http/http_response.h"
#include "net_io/server/generic_server.h"

namespace lt {
namespace net {

struct NoneHttpCoroServerTraits {
  static const bool coro_process = false;
  static const uint64_t kClientConnLimit = 65536;
  static const uint64_t kRequestQpsLimit = 100000;
};

using HttpCoroServer =
  BaseServer<HttpRequestCtx, DefaultConfigurator>;

using HttpServer =
  BaseServer<HttpRequestCtx, NoneHttpCoroServerTraits>;

}}
#endif
