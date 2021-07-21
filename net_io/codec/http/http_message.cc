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

#include "http_message.h"

#include <sstream>

#include <base/base_constants.h>
#include <base/utils/string/str_utils.h>
#include "http_constants.h"
#include "http_parser/http_parser.h"

namespace lt {
namespace net {

HttpMessage::HttpMessage()
  : keepalive_(false),
    http_major_(1),
    http_minor_(1) {}

bool HttpMessage::HasHeader(const char* f) const {
  return headers_.find(f) != headers_.end();
}

bool HttpMessage::HasHeader(const std::string& field) const {
  return headers_.find(field) != headers_.end();
}

void HttpMessage::InsertHeader(const char* k, const char* v) {
  headers_.insert(std::make_pair(k, v));
}

void HttpMessage::InsertHeader(const std::pair<std::string, std::string>&& kv) {
  if (kv.first.empty())
    return;
  headers_.insert(std::move(kv));
}

void HttpMessage::InsertHeader(const std::string& k, const std::string& v) {
  if (k.empty())
    return;
  headers_.insert(std::make_pair(k, v));
}

const std::string& HttpMessage::GetHeader(const std::string& field) const {
  auto iter = headers_.find(field);
  if (iter != headers_.end()) {
    return iter->second;
  }
  return base::kNullString;
}

void HttpMessage::AppendBody(const char* data, size_t len) {
  body_.append(data, len);
}

void HttpMessage::SetBody(std::string&& body) {
  body_ = std::move(body);
}

void HttpMessage::SetBody(const std::string& body) {
  body_ = body;
}

void HttpMessage::SetBody(const char* body) {
  body_ = body;
}

void HttpMessage::SetBody(const char* body, size_t len) {
  body_.clear();
  body_.append(body, len);
}

HttpRequest::HttpRequest()
  : CodecMessage(MessageType::kRequest),
    method_("GET"),
    url_param_parsed_(false) {}

HttpRequest::~HttpRequest() {}

void HttpRequest::AppendPartURL(const char* data, size_t len) {
  url_.append(data, len);
  parse_url_view();
}

void HttpRequest::SetRequestURL(const char* url) {
  url_ = url;
  parse_url_view();
}

void HttpRequest::SetRequestURL(const std::string& url) {
  url_ = url;
  parse_url_view();
}

const std::string& HttpRequest::RequestUrl() const {
  return url_.size() ? url_ : HttpConstant::kRootPath;
}

KeyValMap& HttpRequest::MutableParams() {
  return params_;
}

void HttpRequest::parse_url_view() {
  const std::string& url = RequestUrl();

  struct http_parser_url u;
  http_parser_url_init(&u);
  int result = http_parser_parse_url(url.data(), url.size(), 0, &u);
  if (result) {
    LOG(ERROR) << " http_parser_parse_url error, url:" << url;
    return;
  }
  for (int i = 0; i < UF_MAX; i++) {
    if (!(u.field_set & (1 << i))) {
      continue;
    }
    switch (i) {
      case UF_PATH:
        url_view_.path =
            string_view(url.data() + u.field_data[i].off, u.field_data[i].len);
        break;
      case UF_QUERY:
        url_view_.queries =
            string_view(url.data() + u.field_data[i].off, u.field_data[i].len);
        break;
      case UF_FRAGMENT:
        url_view_.fragment =
            string_view(url.data() + u.field_data[i].off, u.field_data[i].len);
      default:
        break;
    }
  }
}

const std::string& HttpRequest::GetUrlParam(const char* key) {
  const std::string str_key(key);
  return GetUrlParam(str_key);
}

const std::string& HttpRequest::GetUrlParam(const std::string& key) {
  const auto& val = params_.find(key);
  return val != params_.end() ? val->second : base::kNullString;
}

const std::string& HttpRequest::Method() const {
  return method_;
}

const std::string HttpRequest::Dump() const {
  std::ostringstream oss;
  oss << "{\"type\": \"" << TypeAsStr() << "\""
      << ", \"http_major\": " << (int)http_major_
      << ", \"http_minor\": " << (int)http_minor_ << ", \"method\": \""
      << Method() << "\""
      << ", \"request_url\": \"" << url_ << "\"";

  for (const auto& pair : headers_) {
    oss << ", \"header." << pair.first << "\": \"" << pair.second << "\"";
  }
  oss << ", \"keep_alive\": " << keepalive_ << ", \"body\": \"" << body_ << "\""
      << "}";
  return std::move(oss.str());
}

void HttpRequest::SetMethod(const std::string method) {
  method_ = method;
  base::StrUtil::ToUpper(method_);
}

// static
RefHttpResponse HttpResponse::CreateWithCode(uint16_t code) {
  auto r = std::make_shared<HttpResponse>();
  r->SetResponseCode(code);
  return r;
}

HttpResponse::HttpResponse() : CodecMessage(MessageType::kResponse) {}

HttpResponse::~HttpResponse() {}

const std::string HttpResponse::Dump() const {
  std::ostringstream oss;
  oss << "{\"type\": \"" << TypeAsStr() << "\""
      << ", \"http_major\": " << (int)http_major_
      << ", \"http_minor\": " << (int)http_minor_
      << ", \"status_code\": " << (int)status_code_;

  for (const auto& pair : headers_) {
    oss << ", \"header." << pair.first << "\": \"" << pair.second << "\"";
  }
  oss << ", \"keep_alive\": " << keepalive_ << ", \"body\": \"" << body_ << "\""
      << "}";
  return oss.str();
}

uint16_t HttpResponse::ResponseCode() const {
  return status_code_;
}

void HttpResponse::SetResponseCode(uint16_t code) {
  status_code_ = code;
}

std::string HttpResponse::StatusCodeInfo() const {
  const char* desc = http_status_str(http_status(status_code_));
  return desc;
}

}  // namespace net
};  // namespace lt
