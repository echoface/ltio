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

#ifndef NET_HTTP_PROTO_SERVICE_H
#define NET_HTTP_PROTO_SERVICE_H

#include <string>
#include <vector>

#include "net_io/codec/codec_service.h"
#include "net_io/net_callback.h"
#include "parser_context.h"

struct http_parser_settings;

namespace lt {
namespace net {

class HttpCodecService;
typedef std::shared_ptr<HttpCodecService> RefHttpCodecService;

class HttpCodecService : public CodecService {
public:
  HttpCodecService(base::MessageLoop* loop);
  ~HttpCodecService();

  // override from CodecService
  void OnDataFinishSend(const SocketChannel*) override;
  void OnDataReceived(const SocketChannel*, IOBuffer*) override;

  static bool RequestToBuffer(const HttpRequest*, IOBuffer*);
  static bool ResponseToBuffer(const HttpResponse*, IOBuffer*);

  void BeforeSendRequest(HttpRequest*);
  bool SendRequest(CodecMessage* message) override;

  bool BeforeSendResponseMessage(const HttpRequest*, HttpResponse*);
  bool SendResponse(const CodecMessage* req, CodecMessage* res) override;

  const RefCodecMessage NewResponse(const CodecMessage*) override;

private:
  bool UseSSLChannel() const override;
  bool ParseHttpRequest(SocketChannel* channel, IOBuffer*);
  bool ParseHttpResponse(SocketChannel* channel, IOBuffer*);

  ReqParseContext* request_context_;
  ResParseContext* response_context_;
  static http_parser_settings req_parser_settings_;
  static http_parser_settings res_parser_settings_;
};

}  // namespace net
}  // namespace lt
#endif
