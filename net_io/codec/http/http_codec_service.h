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
  /*
  struct http_parser_settings {
  http_cb      on_message_begin;
  http_data_cb on_url;
  http_data_cb on_status;
  http_data_cb on_header_field;
  http_data_cb on_header_value;
  http_cb      on_headers_complete;
  http_data_cb on_body;
  http_cb      on_message_complete;
  //When on_chunk_header is called,
  //the current chunk length is stored
  //in parser->content_length.
  http_cb      on_chunk_header;
  http_cb      on_chunk_complete;
  };*/

  static int OnMessageBegin(http_parser* parser);

  static int OnHeaderFinish(http_parser* parser);

  static int OnMessageEnd(http_parser* parser);

  static int OnChunkHeader(http_parser* parser);

  static int OnChunkFinished(http_parser* parser);

  static int OnURL(http_parser* parser, const char* url, size_t len);

  static int OnStatus(http_parser* parser, const char* start, size_t len);

  static int OnHeaderField(http_parser* parser, const char* field, size_t len);

  static int OnHeaderValue(http_parser* parser, const char* value, size_t len);

  static int OnMessageBody(http_parser* parser, const char* body, size_t len);

  void StartProtocolService() override;

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

private:
  using HeaderKV = std::pair<std::string, std::string>;

  http_parser parser_;

  http_parser_settings settings_;

  HeaderKV half_header;

  RefHttpRequest cur_req_;

  RefHttpResponse cur_res_;

  RefHttpResponse current_;

  std::list<RefHttpResponse> messages_;

  ReqParseContext* request_context_;

  ResParseContext* response_context_;

  static http_parser_settings req_parser_settings_;

  static http_parser_settings res_parser_settings_;
};

}  // namespace net
}  // namespace lt
#endif
