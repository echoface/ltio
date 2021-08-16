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

#ifndef _NET_HTTP_SERVER_CONTEXT_H_H
#define _NET_HTTP_SERVER_CONTEXT_H_H

#include "net_io/codec/http/http_codec_service.h"

namespace lt {
namespace net {

class H2Context;
class HttpRequestCtx;

using RefH2Context = std::shared_ptr<H2Context>;
using RefHttpRequestCtx = std::shared_ptr<HttpRequestCtx>;

class HttpRequestCtx {
public:
  static RefHttpRequestCtx New(const RefCodecMessage& req);

  const HttpRequest* Request() { return (HttpRequest*)request_.get(); }

  void File(const std::string&, uint16_t code = 200);

  void Json(const std::string& json, uint16_t code = 200);

  void String(const char* content, uint16_t code = 200);

  void String(const std::string& content, uint16_t code = 200);

  void Response(RefHttpResponse& response);

  bool Responsed() const { return did_reply_; };

protected:
  HttpRequestCtx(const RefCodecMessage& request);

private:

  RefCodecMessage request_;

  bool did_reply_ = false;

  base::MessageLoop* io_loop_ = NULL;
};

class H2Context : public HttpRequestCtx {
public:
  static RefH2Context New(const RefCodecMessage& req);

  /*
   * server push api, success notify if success or fail
   *
   * status: 0: success, other: fail code
   * */
  void Push(const std::string& method,
            const std::string& path,
            const HttpRequest* bind_req,
            const RefHttpResponse& resp,
            std::function<void(int status)> callback);
protected:
  H2Context(const RefCodecMessage& request);

private:
  std::vector<RefHttpResponse> srv_pushs_;
};

}  // namespace net
}  // namespace lt

#include "net_io/server/handler_factory.h"

template <typename Functor>
lt::net::CodecService::Handler* NewHttpHandler(const Functor& func) {
  return NewHandler<Functor, lt::net::HttpRequestCtx>(func);
}

template <typename Functor>
lt::net::CodecService::Handler* NewHttpCoroHandler(const Functor& func) {
  return NewCoroHandler<Functor, lt::net::HttpRequestCtx>(func);
}

template <typename Functor>
lt::net::CodecService::Handler* NewHttp2Handler(const Functor& func) {
  return NewHandler<Functor, lt::net::H2Context>(func);
}

template <typename Functor>
lt::net::CodecService::Handler* NewHttp2CoroHandler(const Functor& func) {
  return NewCoroHandler<Functor, lt::net::H2Context>(func);
}

#endif
