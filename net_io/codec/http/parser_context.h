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

#include <memory>
#include <string>

#include <llhttp.h>

#include <base/logging.h>
#include <base/utils/gzip/gzip_utils.h>
#include "http_constants.h"
#include "http_request.h"
#include "http_response.h"
#include "net_io/io_buffer.h"

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
    llhttp_settings_init(&settings_);
    settings_.on_message_begin = &Parser::OnMessageBegin;

    /* Possible return values 0, -1, HPE_USER */
    settings_.on_url = &Parser::OnURL;
    settings_.on_status = &Parser::OnStatus;
    settings_.on_header_field = &Parser::OnHeaderField;
    settings_.on_header_value = &Parser::OnHeaderValue;

    /* Possible return values:
     * 0  - Proceed normally
     * 1  - Assume that request/response has no body, and proceed to parsing
     * the next message
     * 2  - Assume absence of body (as above) and make
     * `llhttp_execute()` return `HPE_PAUSED_UPGRADE`
     * -1 - Error
     * `HPE_PAUSED`
     */
    settings_.on_headers_complete = &Parser::OnHeaderFinish;

    /* Possible return values 0, -1, HPE_USER */
    settings_.on_body = &Parser::OnMessageBody;

    /* Possible return values 0, -1, `HPE_PAUSED` */
    settings_.on_message_complete = &Parser::OnMessageEnd;
    /* Information-only callbacks, return value is ignored */
    settings_.on_url_complete = nullptr;
    settings_.on_status_complete = nullptr;
    settings_.on_header_field_complete = nullptr;
    settings_.on_header_value_complete = nullptr;

    /* When on_chunk_header is called, the current chunk length is stored
     * in parser->content_length.
     * Possible return values 0, -1, `HPE_PAUSED`
     */
    settings_.on_chunk_header = &Parser::OnChunkHeader;
    settings_.on_chunk_complete = &Parser::OnChunkFinished;
    llhttp_init(&parser_, HTTP_BOTH, &settings_);
    parser_.data = this;
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
    llhttp_errno_t err = llhttp_execute(&parser_, buffer_start, buffer_size);
    if (err != HPE_OK) {
      LOG(ERROR) << "parser err:" << llhttp_errno_name(err)
                 << ", reason:" << parser_.reason
                 << ", content:" << buf->StringView();
      return Error;
    }
    VLOG(VTRACE) << "parser success, len:" << buffer_size;
    buf->Consume(buffer_size);
    return Success;
  }

  bool Upgrade() const { return parser_.upgrade; }

  static int OnMessageBegin(llhttp_t* parser) {
    Parser* codec = (Parser*)parser->data;

    codec->half_hdr_field_.clear();
    codec->half_hdr_field_.clear();
    codec->hd_value_flag_ = false;

    codec->current_ = std::make_shared<M>();
    return 0;
  }

  static int OnHeaderFinish(llhttp_t* parser) {
    Parser* codec = (Parser*)parser->data;
    M* current = codec->current_.get();
    if (codec->half_hdr_field_.size()) {
      current->InsertHeader(codec->half_hdr_field_, codec->half_hdr_value_);
    }
    codec->half_hdr_field_.clear();
    codec->half_hdr_value_.clear();
    return 0;
  }

  static int OnMessageEnd(llhttp_t* parser) {
    VLOG(VTRACE) << __FUNCTION__ << " enter";
    Parser* codec = (Parser*)parser->data;
    return codec->OnMessageParsed();
  }

  static int OnChunkHeader(llhttp_t* parser) { return 0; }

  static int OnChunkFinished(llhttp_t* parser) { return 0; }

  static int OnURL(llhttp_t* parser, const char* url, size_t len) {
    Parser* codec = (Parser*)parser->data;
    return codec->AppendURL(url, len);
  }

  static int OnStatus(llhttp_t* parser, const char* start, size_t len) {
    // fill at on message end callback
    return 0;
  }

  static int OnHeaderField(llhttp_t* parser, const char* field, size_t len) {
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

  static int OnHeaderValue(llhttp_t* parser, const char* value, size_t len) {
    Parser* codec = (Parser*)parser->data;
    codec->hd_value_flag_ = true;
    codec->half_hdr_value_.append(value, len);
    return 0;
  }

  static int OnMessageBody(llhttp_t* parser, const char* body, size_t len) {
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
    current_->SetKeepAlive(llhttp_should_keep_alive(&parser_));
    current_->SetMethod(llhttp_method_name(llhttp_method_t(parser_.method)));

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
    current_->keepalive_ = llhttp_should_keep_alive(&parser_);
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

  llhttp_t parser_;
  llhttp_settings_t settings_;

  bool hd_value_flag_ = false;
  std::string half_hdr_field_;
  std::string half_hdr_value_;
};  // namespace net

}  // namespace net
}  // namespace lt
#endif
