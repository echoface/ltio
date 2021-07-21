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

#ifndef _NET_HTTP_MESSAGE_H
#define _NET_HTTP_MESSAGE_H

#include <map>
#include <sstream>

#include <net_io/codec/codec_message.h>

namespace lt {
namespace net {

class HttpRequest;
class HttpResponse;
typedef std::shared_ptr<HttpRequest> RefHttpRequest;
typedef std::shared_ptr<HttpResponse> RefHttpResponse;

typedef std::map<std::string, std::string> KeyValMap;

class HttpMessage {
public:
  HttpMessage();
  virtual ~HttpMessage(){};

  std::string& MutableBody() {return body_;}

  const std::string& Body() const {return body_;}

  void SetBody(const char* body);

  void SetBody(const char* body, size_t len);

  void SetBody(const std::string& body);

  void SetBody(const std::string&& body);

  bool IsKeepAlive() const {return keepalive_;};

  void SetKeepAlive(bool alive) {keepalive_ = alive;}

  KeyValMap& MutableHeaders() {return headers_;}

  const KeyValMap& Headers() const {return headers_;}

  bool HasHeaderField(const char* f) const;

  bool HasHeaderField(const std::string&) const;

  void InsertHeader(const std::string&, const std::string&);

  void InsertHeader(const char*, const char*);

  void InsertHeader(const std::pair<std::string, std::string>&& kv);

  const std::string& GetHeader(const std::string&) const;

  int VersionMajor() const { return http_major_; }

  int VersionMinor() const { return http_minor_; }
protected:
  bool keepalive_ = false;
  uint8_t http_major_ = 1;
  uint8_t http_minor_ = 1;

  std::string body_;
  KeyValMap headers_;
};

class HttpRequest : public HttpMessage, public CodecMessage {
public:
  typedef HttpResponse ResponseType;

  HttpRequest();

  ~HttpRequest();

  void SetMethod(const std::string);

  const std::string& Method() const;

  void SetRequestURL(const char* url);

  void SetRequestURL(const std::string&);

  const std::string& RequestUrl() const;

  KeyValMap& MutableParams();

  const std::string& GetUrlParam(const char*);

  const std::string& GetUrlParam(const std::string&);

  const std::string Dump() const override;

private:
  void ParseUrlToParams();

private:
  friend class HttpCodecService;
  friend class ReqParseContext;

  std::string method_;

  std::string url_;

  KeyValMap params_;
  /*indicate url has parsed to params_*/
  bool url_param_parsed_ = false;
};

class HttpResponse : public HttpMessage, public CodecMessage {
public:
  HttpResponse();

  ~HttpResponse();

  static RefHttpResponse CreateWithCode(uint16_t code);

  const std::string Dump() const override;

  uint16_t ResponseCode() const;

  void SetResponseCode(uint16_t code);

  std::string StatusCodeInfo() const;

private:
  friend class HttpCodecService;
  friend class ResParseContext;

  bool keepalive_;
  uint8_t http_major_;
  uint8_t http_minor_;
  uint16_t status_code_;
};

}  // namespace net
}  // namespace lt
#endif

