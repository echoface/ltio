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

#include "http_request.h"
#include "http_response.h"
#include "thirdparty/http_parser/http_parser.h"

namespace lt {
namespace net {

// response for build HttpResponse
class ResParseContext {
public:
  ResParseContext();

  void reset();
  http_parser* Parser();

  static int OnHttpResponseBegin(http_parser* parser);
  static int OnUrlParsed(http_parser* parser,
                         const char* url_start,
                         size_t url_len);
  static int OnStatusCodeParsed(http_parser* parser,
                                const char* start,
                                size_t len);
  static int OnHeaderFieldParsed(http_parser* parser,
                                 const char* header_start,
                                 size_t len);
  static int OnHeaderValueParsed(http_parser* parser,
                                 const char* value_start,
                                 size_t len);
  static int OnHeaderFinishParsed(http_parser* parser);
  static int OnBodyParsed(http_parser* parser,
                          const char* body_start,
                          size_t len);
  static int OnHttpResponseEnd(http_parser* parser);
  static int OnChunkHeader(http_parser* parser);
  static int OnChunkFinished(http_parser* parser);

private:
  friend class HttpCodecService;
  http_parser parser_;

  bool last_is_header_value;
  std::pair<std::string, std::string> half_header;
  RefHttpResponse current_;
  std::list<RefHttpResponse> messages_;
};

class ReqParseContext {
public:
  ReqParseContext();

  void reset();
  http_parser* Parser();

  static int OnHttpRequestBegin(http_parser* parser);
  static int OnUrlParsed(http_parser* parser,
                         const char* url_start,
                         size_t url_len);
  static int OnStatusCodeParsed(http_parser* parser,
                                const char* start,
                                size_t len);
  static int OnHeaderFieldParsed(http_parser* parser,
                                 const char* header_start,
                                 size_t len);
  static int OnHeaderValueParsed(http_parser* parser,
                                 const char* value_start,
                                 size_t len);
  static int OnHeaderFinishParsed(http_parser* parser);
  static int OnBodyParsed(http_parser* parser,
                          const char* body_start,
                          size_t len);
  static int OnHttpRequestEnd(http_parser* parser);
  static int OnChunkHeader(http_parser* parser);
  static int OnChunkFinished(http_parser* parser);

private:
  friend class HttpCodecService;
  http_parser parser_;

  bool last_is_header_value;
  std::pair<std::string, std::string> half_header;
  RefHttpRequest current_;
  std::list<RefHttpRequest> messages_;
};

}  // namespace net
}  // namespace lt
#endif
