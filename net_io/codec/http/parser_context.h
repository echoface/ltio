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

#ifndef _NET_HTTP_PROTO_PARSER_CONTEXT_H_
#define _NET_HTTP_PROTO_PARSER_CONTEXT_H_

#include <list>
#include <string>
#include <vector>

#include <base/logging.h>
#include <base/utils/gzip/gzip_utils.h>
#include "http_constants.h"
#include "http_request.h"
#include "http_response.h"
#include "net_io/io_buffer.h"
#include "thirdparty/http_parser/http_parser.h"

namespace lt {
namespace net {

// HttpRequest/HttpResponse
template <typename T, typename M>
class HttpParser {
public:
  typedef HttpParser<T, M> Parser;
  using RefMessage = std::shared_ptr<M>;

  enum Status { Success = 0, Error = 1 };

  template <bool is_server = std::is_same<M, HttpRequest>::value>
  HttpParser(T* reciever) : reciever_(reciever) {
    settings_ = {
        .on_message_begin = &Parser::OnMessageBegin,
        .on_url = &Parser::OnURL,
        .on_status = &Parser::OnStatus,
        .on_header_field = &Parser::OnHeaderField,
        .on_header_value = &Parser::OnHeaderValue,
        .on_headers_complete = &Parser::OnHeaderFinish,
        .on_body = &Parser::OnMessageBody,
        .on_message_complete = &Parser::OnMessageEnd,
        .on_chunk_header = &Parser::OnChunkHeader,
        .on_chunk_complete = &Parser::OnChunkFinished,
    };
    parser_.data = this;
    http_parser_init(&parser_, is_server ? HTTP_REQUEST : HTTP_RESPONSE);
  }

  ~HttpParser() {}

  void Reset() {
    current_.reset();
    hd_value_flag_ = false;
    half_hdr_field_.clear();
    half_hdr_value_.clear();
  }

  Status Parse(IOBuffer* buf) {
    size_t buffer_size = buf->CanReadSize();
    const char* buffer_start = (const char*)buf->GetRead();

    size_t nparsed =
        http_parser_execute(&parser_, &settings_, buffer_start, buffer_size);
    VLOG(VTRACE) << "nparsed:" << nparsed << ", bufsize:" << buffer_size
                      << ", content:" << buf->AsString();
    buf->Consume(nparsed);
    if (nparsed != buffer_size) {
      LOG(ERROR) << "http-parser errno:"
                 << http_errno_description(HTTP_PARSER_ERRNO(&parser_));
      return Error;
    }
    return Success;
  }

  bool Upgrade() const { return parser_.upgrade; }

  static int OnMessageBegin(http_parser* parser) {
    Parser* codec = (Parser*)parser->data;

    codec->half_hdr_field_.clear();
    codec->half_hdr_field_.clear();
    codec->hd_value_flag_ = false;

    codec->current_ = std::make_shared<M>();
    return 0;
  }

  static int OnHeaderFinish(http_parser* parser) {
    Parser* codec = (Parser*)parser->data;
    M* current = codec->current_.get();
    if (codec->half_hdr_field_.size()) {
      current->InsertHeader(codec->half_hdr_field_, codec->half_hdr_value_);
    }
    codec->half_hdr_field_.clear();
    codec->half_hdr_value_.clear();
    return 0;
  }

  static int OnMessageEnd(http_parser* parser) {
    VLOG(VTRACE) << __FUNCTION__ << " enter";
    Parser* codec = (Parser*)parser->data;
    return codec->OnMessageParsed();
  }

  static int OnChunkHeader(http_parser* parser) { return 0; }

  static int OnChunkFinished(http_parser* parser) { return 0; }

  static int OnURL(http_parser* parser, const char* url, size_t len) {
    Parser* codec = (Parser*)parser->data;
    return codec->AppendURL(url, len);
  }

  static int OnStatus(http_parser* parser, const char* start, size_t len) {
    // fill at on message end callback
    return 0;
  }

  static int OnHeaderField(http_parser* parser, const char* field, size_t len) {
    Parser* codec = (Parser*)parser->data;
    M* current = codec->current_.get();
    if (codec->hd_value_flag_) {
      current->InsertHeader(codec->half_hdr_field_, codec->half_hdr_value_);
      codec->half_hdr_field_.clear();
      codec->half_hdr_value_.clear();
    }
    codec->hd_value_flag_ = false;
    codec->half_hdr_field_.append(field, len);
    return 0;
  }

  static int OnHeaderValue(http_parser* parser, const char* value, size_t len) {
    Parser* codec = (Parser*)parser->data;
    codec->hd_value_flag_ = true;
    codec->half_hdr_value_.append(value, len);
    return 0;
  }

  static int OnMessageBody(http_parser* parser, const char* body, size_t len) {
    Parser* codec = (Parser*)parser->data;
    codec->current_->AppendBody(body, len);
    return 0;
  }

  template <typename U = M,
            typename std::enable_if<
                std::is_same<U, HttpRequest>::value>::type* = nullptr>
  int AppendURL(const char* url, size_t len) {
    current_->AppendPartURL(url, len);
    return 0;
  }

  template <typename U = M,
            typename std::enable_if<
                std::is_same<U, HttpResponse>::value>::type* = nullptr>
  int AppendURL(const char* url, size_t len) {
    LOG(ERROR) << __FUNCTION__ << "response version reached";
    return -1;
  }

  template <typename U = M,
            typename std::enable_if<
                std::is_same<U, HttpRequest>::value>::type* = nullptr>
  int OnMessageParsed() {
    // complete message and commit to service handler
    current_->http_major_ = parser_.http_major;
    current_->http_minor_ = parser_.http_minor;
    current_->SetKeepAlive(http_should_keep_alive(&parser_));
    current_->SetMethod(http_method_str((http_method)parser_.method));

    // gzip, inflat
    const std::string kgzip("gzip");
    const std::string& encoding =
        current_->GetHeader(HttpConstant::kContentEncoding);
    if (encoding.find(kgzip) != std::string::npos) {
      std::string decompress_body;
      if (0 != base::Gzip::decompress_gzip(current_->Body(), decompress_body)) {
        LOG(ERROR) << " Decode gzip HttpRequest Body Failed";
        return -1;
      }
      current_->SetBody(std::move(decompress_body));
    } else if (encoding.find("deflate") != std::string::npos) {
      std::string decompress_body;
      if (0 !=
          base::Gzip::decompress_deflate(current_->Body(), decompress_body)) {
        LOG(ERROR) << " Decode deflate HttpRequest Body Failed";
        return -1;
      }
      current_->SetBody(std::move(decompress_body));
    }

    reciever_->CommitHttpRequest(std::move(current_));
    return 0;
  }

  template <typename U = M,
            typename std::enable_if<
                std::is_same<U, HttpResponse>::value>::type* = nullptr>
  int OnMessageParsed() {
    // complete message and commit to service handler
    current_->keepalive_ = http_should_keep_alive(&parser_);
    current_->http_major_ = parser_.http_major;
    current_->http_minor_ = parser_.http_minor;
    current_->status_code_ = parser_.status_code;

    const std::string& encoding =
        current_->GetHeader(HttpConstant::kContentEncoding);

    if (encoding.find("gzip") != std::string::npos) {
      std::string decompress_body;
      if (0 != base::Gzip::decompress_gzip(current_->Body(), decompress_body)) {
        LOG(ERROR) << " Decode gzip HttpRequest Body Failed";
        return -1;
      }
      current_->SetBody(std::move(decompress_body));
      current_->RemoveHeader(HttpConstant::kContentEncoding);

    } else if (encoding.find("deflate") != std::string::npos) {
      std::string decompress_body;
      if (0 !=
          base::Gzip::decompress_deflate(current_->Body(), decompress_body)) {
        LOG(ERROR) << " Decode deflate HttpRequest Body Failed";
        return -1;
      }
      current_->SetBody(std::move(decompress_body));
      current_->RemoveHeader(HttpConstant::kContentEncoding);
    }
    reciever_->CommitHttpResponse(std::move(current_));
    return 0;
  }

protected:
  T* reciever_ = nullptr;

  RefMessage current_;

  http_parser parser_;

  http_parser_settings settings_;

  bool hd_value_flag_ = false;
  std::string half_hdr_field_;
  std::string half_hdr_value_;
};

}  // namespace net
}  // namespace lt
#endif
