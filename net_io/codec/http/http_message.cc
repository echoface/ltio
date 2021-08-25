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

#include "base/logging.h"
#include "base/utils/string/str_utils.h"
#include "http_constants.h"
#include "url-parser/url_parser.h"

namespace lt {
namespace net {

HttpMessage::HttpMessage()
  : CodecMessage(),
    keepalive_(false),
    http_major_(1),
    http_minor_(1) {}

bool HttpMessage::HasHeader(const char* f) const {
  std::string field(f);
  base::StrUtil::ToLower(field);
  return headers_.find(field) != headers_.end();
}

bool HttpMessage::HasHeader(const std::string& field) const {
  std::string fieldlowcase(field);
  base::StrUtil::ToLower(fieldlowcase);
  return headers_.find(fieldlowcase) != headers_.end();
}

void HttpMessage::InsertHeader(const char* k, const char* v) {
  std::string field(k);
  base::StrUtil::ToLower(field);
  // std::string value(v); base::StrUtil::ToLower(value);
  headers_.insert(std::make_pair(field, v));
}

void HttpMessage::InsertHeader(const std::pair<std::string, std::string>&& kv) {
  if (kv.first.empty())
    return;
  std::string field(kv.first);
  base::StrUtil::ToLower(field);
  // std::string value(kv.second); base::StrUtil::ToLower(value);
  headers_.insert(std::make_pair(std::move(field), std::move(kv.second)));
}

void HttpMessage::InsertHeader(const std::string& k, const std::string& v) {
  if (k.empty())
    return;

  std::string field(k);
  base::StrUtil::ToLower(field);
  // std::string value(v); base::StrUtil::ToLower(value);
  headers_.insert(std::make_pair(std::move(field), v));
}

const std::string& HttpMessage::GetHeader(const std::string& field) const {
  std::string fieldlowcase = field;
  base::StrUtil::ToLower(fieldlowcase);
  auto iter = headers_.find(fieldlowcase);
  if (iter != headers_.end()) {
    return iter->second;
  }
  return base::EmptyString;
}

bool HttpMessage::RemoveHeader(const std::string& field) {
  std::string fieldlowcase = field;
  base::StrUtil::ToLower(fieldlowcase);
  return headers_.erase(fieldlowcase);
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

HttpRequest::HttpRequest() : method_("GET"), url_param_parsed_(false) {}

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
  return val != params_.end() ? val->second : base::EmptyString;
}

const std::string& HttpRequest::Method() const {
  return method_;
}

const std::string HttpRequest::Dump() const {
  std::ostringstream oss;
  oss << "{ \"major\": " << (int)http_major_
      << ", \"minor\": " << (int)http_minor_
      << ", \"alive\": " << (int)keepalive_ << ", \"method\": \"" << Method()
      << "\""
      << ", \"url\": \"" << url_ << "\"";
  for (const auto& pair : headers_) {
    oss << ", \"header." << pair.first << "\": \"" << pair.second << "\"";
  }
  oss << ", \"body\": \"" << body_ << "\"}";
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

HttpResponse::HttpResponse() {}

HttpResponse::~HttpResponse() {}

const std::string HttpResponse::Dump() const {
  std::ostringstream oss;
  oss << "{ \"major\": " << (int)http_major_
      << ", \"minor\": " << (int)http_minor_
      << ", \"code\": " << (int)status_code_;

  for (const auto& pair : headers_) {
    oss << ", \"header." << pair.first << "\": \"" << pair.second << "\"";
  }
  oss << ", \"body\": \"" << body_ << "\"}";
  return oss.str();
}

uint16_t HttpResponse::ResponseCode() const {
  return status_code_;
}

void HttpResponse::SetResponseCode(uint16_t code) {
  status_code_ = code;
}

std::string HttpResponse::StatusCodeInfo() const {
  return http_status_desc(status_code_).to_string();
}

}  // namespace net
};  // namespace lt
