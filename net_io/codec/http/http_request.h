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

#ifndef _NET_HTTP_PROTO_MESSAGE_H
#define _NET_HTTP_PROTO_MESSAGE_H

#include <map>
#include <sstream>
#include "http_response.h"

#include <net_io/codec/codec_message.h>

namespace lt {
namespace net {

class HttpRequest;

typedef std::map<std::string, std::string> KeyValMap;
typedef std::shared_ptr<HttpRequest> RefHttpRequest;

class HttpRequest : public CodecMessage {
public:
  typedef HttpResponse ResponseType;
  HttpRequest();
  ~HttpRequest();

  std::string& MutableBody();
  const std::string& Body() const;

  void SetMethod(const std::string);
  const std::string& Method() const;

  void SetRequestURL(const char* url);
  void SetRequestURL(const std::string&);
  const std::string& RequestUrl() const;

  KeyValMap& MutableHeaders();
  const KeyValMap& Headers() const;
  bool HasHeaderField(const char*) const;
  bool HasHeaderField(const std::string&) const;
  void InsertHeader(const char*, const char*);
  void InsertHeader(const std::string&, const std::string&);
  const std::string& GetHeader(const std::string&) const;

  KeyValMap& MutableParams();
  const std::string& GetUrlParam(const char*);
  const std::string& GetUrlParam(const std::string&);

  bool IsKeepAlive() const;
  void SetKeepAlive(bool alive);

  int VersionMajor() const { return http_major_; }
  int VersionMinor() const { return http_minor_; }

  const std::string Dump() const override;

private:
  void ParseUrlToParams();

private:
  friend class HttpCodecService;
  friend class ReqParseContext;

  bool keepalive_;
  int array_[10];
  std::string method_;
  std::string url_;
  std::string body_;

  uint8_t http_major_;
  uint8_t http_minor_;

  KeyValMap headers_;

  KeyValMap params_;
  /*indicate url has parsed to params_*/
  bool url_param_parsed_;
};

}  // namespace net
}  // namespace lt
#endif
