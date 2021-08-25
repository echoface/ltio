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

#include "http_constants.h"

#include <array>
#include <vector>

#include "base/memory/lazy_instance.h"

namespace lt {
namespace net {

// static
const std::string HttpConstant::kRootPath = "/";
const std::string HttpConstant::kCRCN = "\r\n";
const std::string HttpConstant::kHost = "Host";
const std::string HttpConstant::kBlankSpace = " ";
const std::string HttpConstant::kConnection = "Connection";
const std::string HttpConstant::kContentType = "Content-Type";
const std::string HttpConstant::kContentLength = "Content-Length";
const std::string HttpConstant::kContentEncoding = "Content-Encoding";
const std::string HttpConstant::kAcceptEncoding = "Accept-Encoding";

// all default full header and response
const std::string HttpConstant::kBadRequest =
    "HTTP/1.1 400 Bad Request\r\n\r\n";

const std::string HttpConstant::kHeaderKeepAlive = "Connection: keep-alive\r\n";
const std::string HttpConstant::kHeaderNotKeepAlive = "Connection: Close\r\n";
const std::string HttpConstant::kHeaderGzipEncoding =
    "Content-Encoding: gzip\r\n";
const std::string HttpConstant::kHeaderDefaultContentType =
    "Content-Type: text/plain\r\n";
const std::string HttpConstant::kHeaderSupportedEncoding =
    "Accept-Encoding: deflate,gzip\r\n";

namespace {
using StatusTable = std::array<const char*, 512>;
void init_status_str(StatusTable& s) {
  for (int i = 0; i < s.size(); ++i) {
    s[i] = "";
  }
  s[100] = "Continue";
  s[101] = "Switching Protocols";
  s[200] = "OK";
  s[201] = "Created";
  s[202] = "Accepted";
  s[203] = "Non-authoritative Information";
  s[204] = "No Content";
  s[205] = "Reset Content";
  s[206] = "Partial Content";
  s[300] = "Multiple Choices";
  s[301] = "Moved Permanently";
  s[302] = "Found";
  s[303] = "See Other";
  s[304] = "Not Modified";
  s[305] = "Use Proxy";
  s[307] = "Temporary Redirect";
  s[400] = "Bad Request";
  s[401] = "Unauthorized";
  s[402] = "Payment Required";
  s[403] = "Forbidden";
  s[404] = "Not Found";
  s[405] = "Method Not Allowed";
  s[406] = "Not Acceptable";
  s[407] = "Proxy Authentication Required";
  s[408] = "Request Timeout";
  s[409] = "Conflict";
  s[410] = "Gone";
  s[411] = "Length Required";
  s[412] = "Precondition Failed";
  s[413] = "Payload Too Large";
  s[414] = "Request-URI Too Long";
  s[415] = "Unsupported Media Type";
  s[416] = "Requested Range Not Satisfiable";
  s[417] = "Expectation Failed";
  s[500] = "Internal Server Error";
  s[501] = "Not Implemented";
  s[502] = "Bad Gateway";
  s[503] = "Service Unavailable";
  s[504] = "Gateway Timeout";
  s[505] = "HTTP Version Not Supported";
}

void init_status_tailer_table(StatusTable& s) {
  for (int i = 0; i < s.size(); ++i)
    s[i] = "";

  s[100] = "100 Continue";
  s[101] = "101 Switching Protocols";
  s[200] = "200 OK";
  s[201] = "201 Created";
  s[202] = "202 Accepted";
  s[203] = "203 Non-authoritative Information";
  s[204] = "204 No Content";
  s[205] = "205 Reset Content";
  s[206] = "206 Partial Content";
  s[300] = "300 Multiple Choices";
  s[301] = "301 Moved Permanently";
  s[302] = "302 Found";
  s[303] = "303 See Other";
  s[304] = "304 Not Modified";
  s[305] = "305 Use Proxy";
  s[307] = "307 Temporary Redirect";
  s[400] = "400 Bad Request";
  s[401] = "401 Unauthorized";
  s[402] = "402 Payment Required";
  s[403] = "403 Forbidden";
  s[404] = "404 Not Found";
  s[405] = "405 Method Not Allowed";
  s[406] = "406 Not Acceptable";
  s[407] = "407 Proxy Authentication Required";
  s[408] = "408 Request Timeout";
  s[409] = "409 Conflict";
  s[410] = "410 Gone";
  s[411] = "411 Length Required";
  s[412] = "412 Precondition Failed";
  s[413] = "413 Payload Too Large";
  s[414] = "414 Request-URI Too Long";
  s[415] = "415 Unsupported Media Type";
  s[416] = "416 Requested Range Not Satisfiable";
  s[417] = "417 Expectation Failed";
  s[500] = "500 Internal Server Error";
  s[501] = "501 Not Implemented";
  s[502] = "502 Bad Gateway";
  s[503] = "503 Service Unavailable";
  s[504] = "504 Gateway Timeout";
  s[505] = "505 HTTP Version Not Supported";
}

}  // namespace

class HttpStatusUtil {
public:
  HttpStatusUtil() {
    init_status_str(desc);
    init_status_str(resp_tailer);
  }
  std::string_view Desc(int code) {
    return (100 <= code && code <= 511) ? desc[code] : desc[500];
  }

  std::string_view RespTailer(int code) {
    return (100 <= code && code <= 511) ? resp_tailer[code] : resp_tailer[500];
  }
private:
  StatusTable desc;
  StatusTable resp_tailer;
};

base::LazyInstance<HttpStatusUtil> status_util; 

std::string_view http_status_desc(int32_t code) {
  return status_util.get().Desc(code).data();
}

std::string_view http_status_tailer(int32_t code) {
  return status_util.get().RespTailer(code).data();
}

// format: "200 OK\r\n"
const char* HttpConstant::GetResponseStatusTail(int n) {
  return status_util.get().Desc(n).data();
}

}  // namespace net
}  // namespace lt
