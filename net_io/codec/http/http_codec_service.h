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

#include "http_parser/http_parser.h"
#include "net_io/codec/codec_service.h"
#include "net_io/codec/http/http_message.h"
#include "net_io/codec/http/parser_context.h"
#include "net_io/net_callback.h"

struct http_parser_settings;

namespace lt {
namespace net {

class HttpCodecService;
typedef std::shared_ptr<HttpCodecService> RefHttpCodecService;

class HttpCodecService : public CodecService {
public:
  HttpCodecService(base::MessageLoop* loop);

  ~HttpCodecService();

  static bool RequestToBuffer(const HttpRequest*, IOBuffer*);

  static bool ResponseToBuffer(const HttpResponse*, IOBuffer*);

  void StartProtocolService() override;

  void OnDataReceived(IOBuffer*) override;

  // last change for codec modify request 
  void BeforeSendRequest(HttpRequest*);

  bool SendRequest(CodecMessage* message) override;

  // last change for codec modify response
  bool BeforeSendResponse(const HttpRequest*, HttpResponse*);

  bool SendResponse(const CodecMessage* req, CodecMessage* res) override;

  const RefCodecMessage NewResponse(const CodecMessage*) override;

  void CommitHttpRequest(const RefHttpRequest&& request);

  void CommitHttpResponse(const RefHttpResponse&& response);

private:
  bool UseSSLChannel() const override;

  void init_http_parser();

  void finalize_http_parser();

private:
  using HeaderKV = std::pair<std::string, std::string>;
  using HttpReqParser = HttpParser<HttpCodecService, HttpRequest>;
  using HttpResParser = HttpParser<HttpCodecService, HttpResponse>;

  HttpReqParser* req_parser() {
    return http_parser_.req_parser;
  }
  HttpResParser* rsp_parser() {
    return http_parser_.res_parser;
  }
  union Parser {
    HttpReqParser* req_parser;
    HttpResParser* res_parser;
  };

  Parser http_parser_;
};

}  // namespace net
}  // namespace lt
#endif
