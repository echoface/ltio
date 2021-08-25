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

#include "http_codec_service.h"

#include <base/message_loop/message_loop.h>
#include <base/utils/gzip/gzip_utils.h>
#include <net_io/codec/codec_factory.h>
#include <net_io/codec/codec_message.h>
#include <net_io/io_buffer.h>
#include <net_io/tcp_channel.h>

#include "fmt/core.h"
#include "glog/logging.h"
#include "http_constants.h"

namespace lt {
namespace net {

namespace {
static const int32_t kMeanHeaderSize = 32;
static const int32_t kHttpMsgReserveSize = 512;
static const int32_t kCompressionThreshold = 8096;

const char* kHTTP_RESPONSE_HEADER_1_1 = "HTTP/1.1";
const char* kHTTP_RESPONSE_HEADER_1_0 = "HTTP/1.0";

}  // namespace

HttpCodecService::HttpCodecService(base::MessageLoop* loop)
  : CodecService(loop) {

  flush_fn_ = [this]() {
    TryFlushChannel();
    flush_scheduled_ = false;
  };
}

HttpCodecService::~HttpCodecService() {
  finalize_http_parser();
}

bool HttpCodecService::UseSSLChannel() const {
  return protocol_ == "https" ? true : false;
}

void HttpCodecService::StartProtocolService() {
  init_http_parser();

  CodecService::StartProtocolService();
}

void HttpCodecService::OnDataReceived(IOBuffer* buf) {
  VLOG(VTRACE) << __FUNCTION__ << " buffer_size:" << buf->CanReadSize();

  bool success = false;
  if (IsServerSide()) {
    auto parser = req_parser();
    success = parser->Parse(buf) == HttpReqParser::Success;
  } else {
    auto parser = req_parser();
    success = parser->Parse(buf) == HttpReqParser::Success;
  }

  if (!success) {
    ignore_result(channel_->Send(HttpConstant::kBadRequest));
    CloseService(true);  // no callback here
    NotifyCodecClosed();
  }
}

// static
bool HttpCodecService::RequestToBuffer(const HttpRequest* request,
                                       IOBuffer* buffer) {
  CHECK(request && buffer);
  int32_t guess_size = kHttpMsgReserveSize + request->Body().size();
  guess_size += request->Headers().size() * kMeanHeaderSize;
  buffer->EnsureWritableSize(guess_size);

  buffer->WriteString(fmt::format("{} {} HTTP/1.{}\r\n",
                                  request->Method(),
                                  request->RequestUrl(),
                                  request->VersionMinor()));

  for (const auto& header : request->Headers()) {
    buffer->WriteString(fmt::format("{}: {}\r\n", header.first, header.second));
  }

  if (!request->HasHeader(HttpConstant::kConnection)) {
    buffer->WriteString(request->IsKeepAlive()
                            ? HttpConstant::kHeaderKeepalive
                            : HttpConstant::kHeaderClose);
  }

  if (!request->HasHeader(HttpConstant::kAcceptEncoding)) {
    buffer->WriteString(HttpConstant::kHeaderSupportedEncoding);
  }

  if (!request->HasHeader(HttpConstant::kContentLength)) {
    buffer->WriteString(fmt::format("{}: {}\r\n",
                                    HttpConstant::kContentLength,
                                    request->Body().size()));
  }

  if (!request->HasHeader(HttpConstant::kContentType)) {
    buffer->WriteString(HttpConstant::kHeaderDefaultContentType);
  }
  buffer->WriteString(HttpConstant::kCRCN);

  if (request->Body().size()) {
    buffer->WriteString(request->Body());
  }
  return true;
}

bool HttpCodecService::SendRequest(CodecMessage* message) {
  auto request = static_cast<HttpRequest*>(message);
  // last chance manipulate request
  BeforeSendRequest(request);

  if (!RequestToBuffer(request, channel_->WriterBuffer())) {
    LOG(ERROR) << __FUNCTION__ << " failed encode:" << request->Dump();
    return false;
  }
  VLOG(VTRACE) << __FUNCTION__ << ", write request:" << request->Dump();
  if (!flush_scheduled_) {
    flush_scheduled_ = true;
    loop_->PostTask(NewClosure(flush_fn_));
  }
  return true;
  //return TryFlushChannel();
}

bool HttpCodecService::SendResponse(const CodecMessage* req,
                                    CodecMessage* res) {
  HttpResponse* response = static_cast<HttpResponse*>(res);
  const HttpRequest* request = static_cast<const HttpRequest*>(req);

  BeforeSendResponse(request, response);

  if (!ResponseToBuffer(response, channel_->WriterBuffer())) {
    LOG(ERROR) << __FUNCTION__ << " failed encode:" << response->Dump();
    return false;
  }
  VLOG(VTRACE) << "write response:" << response->Dump();
  VLOG(VTRACE) << "response encode buf:\n"
                    << channel_->WriterBuffer()->AsString();
  /* see: https://tools.ietf.org/html/rfc7230#section-6.1

   The "close" connection option is defined for a sender to signal that
   this connection will be closed after completion of the response.  For
   example,

     Connection: close

   in either the request or the response header fields indicates that
   the sender is going to close the connection after the current
   request/response is complete (Section 6.6).
   * */
  if (!response->IsKeepAlive()) {
    schedule_close_ = true;
  }

  if (!flush_scheduled_) {
    flush_scheduled_ = true;
    loop_->PostTask(NewClosure(flush_fn_));
  }
  //return TryFlushChannel();
  return true;
}

// static
bool HttpCodecService::ResponseToBuffer(const HttpResponse* response,
                                        IOBuffer* buffer) {
  CHECK(response && buffer);

  int32_t guess_size = kHttpMsgReserveSize + response->Body().size();
  buffer->EnsureWritableSize(guess_size);

  int32_t code = response->ResponseCode();
  buffer->WriteString(http_resp_head_line(code, response->VersionMinor()));
  // header: value
  for (const auto& header : response->Headers()) {
    buffer->WriteString(fmt::format("{}: {}\r\n", header.first, header.second));
  }

  if (!response->HasHeader(HttpConstant::kConnection)) {
    buffer->WriteString(response->IsKeepAlive()
                            ? HttpConstant::kHeaderKeepalive
                            : HttpConstant::kHeaderClose);
  }

  if (!response->HasHeader(HttpConstant::kContentLength)) {
    buffer->WriteString(fmt::format("{}: {:d}\r\n",
                                    HttpConstant::kContentLength,
                                    response->Body().size()));
  }

  if (!response->HasHeader(HttpConstant::kContentType)) {
    buffer->WriteString(HttpConstant::kHeaderDefaultContentType);
  }
  buffer->WriteString(HttpConstant::kCRCN);

  buffer->WriteString(response->Body());
  return true;
}

const RefCodecMessage HttpCodecService::NewResponse(
    const CodecMessage* request) {
  // static_cast<HttpRequest*>(request);
  const HttpRequest* http_request = (HttpRequest*)request;

  RefHttpResponse http_res = HttpResponse::CreateWithCode(500);

  http_res->SetKeepAlive(http_request->IsKeepAlive());
  http_res->InsertHeader("Content-Type", "text/plain");
  return std::move(http_res);
}

void HttpCodecService::BeforeSendRequest(HttpRequest* out_message) {
  HttpRequest* request = static_cast<HttpRequest*>(out_message);
  if (request->Body().size() > kCompressionThreshold &&
      !request->HasHeader(HttpConstant::kContentEncoding)) {
    std::string compressed_body;
    if (0 == base::Gzip::compress_gzip(request->Body(),
                                       compressed_body)) {  // success
      request->InsertHeader(HttpConstant::kContentEncoding, "gzip");
      request->InsertHeader(HttpConstant::kContentLength,
                            std::to_string(compressed_body.size()));
      request->body_ = std::move(compressed_body);
    }
  }

  if (!out_message->HasHeader(HttpConstant::kHost)) {
    const url::RemoteInfo* remote = delegate_->GetRemoteInfo();
    request->InsertHeader(HttpConstant::kHost, remote->host);
  }
}

bool HttpCodecService::BeforeSendResponse(const HttpRequest* request,
                                          HttpResponse* response) {
  // response compression if needed
  if (response->Body().size() > kCompressionThreshold &&
      !response->HasHeader(HttpConstant::kContentEncoding)) {
    const std::string& accept =
        request->GetHeader(HttpConstant::kAcceptEncoding);
    std::string compressed_body;
    if (accept.find("gzip") != std::string::npos) {
      if (0 == base::Gzip::compress_gzip(response->Body(),
                                         compressed_body)) {  // success
        response->body_ = std::move(compressed_body);
        response->InsertHeader("Content-Encoding", "gzip");
      }
    } else if (accept.find("deflate") != std::string::npos) {
      if (0 ==
          base::Gzip::compress_deflate(response->Body(), compressed_body)) {
        response->body_ = std::move(compressed_body);
        response->InsertHeader("Content-Encoding", "deflate");
      }
    }
  }
  return true;
}

void HttpCodecService::CommitHttpRequest(const RefHttpRequest&& request) {
  request->SetIOCtx(shared_from_this());
  handler_->OnCodecMessage(std::move(request));
}

void HttpCodecService::CommitHttpResponse(const RefHttpResponse&& response) {
  response->SetIOCtx(shared_from_this());
  handler_->OnCodecMessage(std::move(response));
}

void HttpCodecService::init_http_parser() {
  if (IsServerSide()) {
    http_parser_.req_parser = new HttpReqParser(this);
  } else {
    http_parser_.res_parser = new HttpResParser(this);
  }
}

void HttpCodecService::finalize_http_parser() {
  if (IsServerSide()) {
    delete http_parser_.req_parser;
    http_parser_.req_parser = nullptr;
  } else {
    delete http_parser_.res_parser;
    http_parser_.res_parser = nullptr;
  }
}

}  // namespace net
}  // namespace lt
