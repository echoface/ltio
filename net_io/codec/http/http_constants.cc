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
#include "fmt/core.h"

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
const std::string HttpConstant::kHeaderClose = "Connection: Close\r\n";
const std::string HttpConstant::kHeaderKeepalive = "Connection: keep-alive\r\n";
const std::string HttpConstant::kHeaderGzipEncoding =
    "Content-Encoding: gzip\r\n";
const std::string HttpConstant::kHeaderDefaultContentType =
    "Content-Type: text/plain\r\n";
const std::string HttpConstant::kHeaderSupportedEncoding =
    "Accept-Encoding: deflate,gzip\r\n";

namespace {
using StatusTable = std::array<std::string, 512>;
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

void init_http_resp_table(StatusTable& s, uint8_t subversion) {
  for (int i = 0; i < s.size(); ++i)
    s[i] = "";
  std::string prefix = fmt::format("HTTP/1.{:d} ", subversion);
  s[100] = prefix + "100 Continue\r\n";
  s[101] = prefix + "101 Switching Protocols\r\n";
  s[200] = prefix + "200 OK\r\n";
  s[201] = prefix + "201 Created\r\n";
  s[202] = prefix + "202 Accepted\r\n";
  s[203] = prefix + "203 Non-authoritative Information\r\n";
  s[204] = prefix + "204 No Content\r\n";
  s[205] = prefix + "205 Reset Content\r\n";
  s[206] = prefix + "206 Partial Content\r\n";
  s[300] = prefix + "300 Multiple Choices\r\n";
  s[301] = prefix + "301 Moved Permanently\r\n";
  s[302] = prefix + "302 Found\r\n";
  s[303] = prefix + "303 See Other\r\n";
  s[304] = prefix + "304 Not Modified\r\n";
  s[305] = prefix + "305 Use Proxy\r\n";
  s[307] = prefix + "307 Temporary Redirect\r\n";
  s[400] = prefix + "400 Bad Request\r\n";
  s[401] = prefix + "401 Unauthorized\r\n";
  s[402] = prefix + "402 Payment Required\r\n";
  s[403] = prefix + "403 Forbidden\r\n";
  s[404] = prefix + "404 Not Found\r\n";
  s[405] = prefix + "405 Method Not Allowed\r\n";
  s[406] = prefix + "406 Not Acceptable\r\n";
  s[407] = prefix + "407 Proxy Authentication Required\r\n";
  s[408] = prefix + "408 Request Timeout\r\n";
  s[409] = prefix + "409 Conflict\r\n";
  s[410] = prefix + "410 Gone\r\n";
  s[411] = prefix + "411 Length Required\r\n";
  s[412] = prefix + "412 Precondition Failed\r\n";
  s[413] = prefix + "413 Payload Too Large\r\n";
  s[414] = prefix + "414 Request-URI Too Long\r\n";
  s[415] = prefix + "415 Unsupported Media Type\r\n";
  s[416] = prefix + "416 Requested Range Not Satisfiable\r\n";
  s[417] = prefix + "417 Expectation Failed\r\n";
  s[500] = prefix + "500 Internal Server Error\r\n";
  s[501] = prefix + "501 Not Implemented\r\n";
  s[502] = prefix + "502 Bad Gateway\r\n";
  s[503] = prefix + "503 Service Unavailable\r\n";
  s[504] = prefix + "504 Gateway Timeout\r\n";
  s[505] = prefix + "505 HTTP Version Not Supported\r\n";
}

}  // namespace

class HttpStatusUtil {
public:
  HttpStatusUtil() {
    init_status_str(desc);
    init_http_resp_table(resp_hl[0], 0);
    init_http_resp_table(resp_hl[1], 1);
  }
  const std::string& Desc(int code) {
    return (100 <= code && code <= 511) ? desc[code] : desc[500];
  }

  const std::string& RespHeadLine(int code, uint8_t subversion) {
    if (100 <= code && code <= 511 && subversion <= 1) {
      return resp_hl[subversion][code];
    }
    return resp_hl[subversion][500];
  }

private:
  StatusTable desc;
  std::array<StatusTable, 2> resp_hl;
};

base::LazyInstance<HttpStatusUtil> status_util;

const std::string& http_status_desc(int code) {
  return status_util.get().Desc(code);
}

const std::string& http_resp_head_line(int code, uint8_t subversion) {
  return status_util.get().RespHeadLine(code, subversion);
}

}  // namespace net
}  // namespace lt
