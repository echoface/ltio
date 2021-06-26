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

#include "base/message_loop/message_loop.h"
#include "http_constants.h"
#include "http_codec_service.h"

#include "glog/logging.h"
#include <net_io/io_buffer.h>
#include <net_io/tcp_channel.h>
#include <net_io/codec/codec_message.h>
#include <base/utils/gzip/gzip_utils.h>

namespace lt {
namespace net {

static const int32_t kMeanHeaderSize = 32;
static const int32_t kHttpMsgReserveSize = 64;
static const int32_t kCompressionThreshold = 4096;

//static
http_parser_settings HttpCodecService::req_parser_settings_ = {
  .on_message_begin = &ReqParseContext::OnHttpRequestBegin,
  .on_url = &ReqParseContext::OnUrlParsed,
  .on_status = &ReqParseContext::OnStatusCodeParsed,
  .on_header_field = &ReqParseContext::OnHeaderFieldParsed,
  .on_header_value = &ReqParseContext::OnHeaderValueParsed,
  .on_headers_complete = &ReqParseContext::OnHeaderFinishParsed,
  .on_body = &ReqParseContext::OnBodyParsed,
  .on_message_complete = &ReqParseContext::OnHttpRequestEnd,
  .on_chunk_header = &ReqParseContext::OnChunkHeader,
  .on_chunk_complete = &ReqParseContext::OnChunkFinished,
};

http_parser_settings HttpCodecService::res_parser_settings_ = {
  .on_message_begin = &ResParseContext::OnHttpResponseBegin,
  .on_url = &ResParseContext::OnUrlParsed,
  .on_status = &ResParseContext::OnStatusCodeParsed,
  .on_header_field = &ResParseContext::OnHeaderFieldParsed,
  .on_header_value = &ResParseContext::OnHeaderValueParsed,
  .on_headers_complete = &ResParseContext::OnHeaderFinishParsed,
  .on_body = &ResParseContext::OnBodyParsed,
  .on_message_complete = &ResParseContext::OnHttpResponseEnd,
  .on_chunk_header = &ResParseContext::OnChunkHeader,
  .on_chunk_complete = &ResParseContext::OnChunkFinished,
};

HttpCodecService::HttpCodecService(base::MessageLoop* loop)
  : CodecService(loop),
    request_context_(nullptr),
    response_context_(nullptr) {

  request_context_ = new ReqParseContext();
  response_context_ = new ResParseContext();
}

HttpCodecService::~HttpCodecService() {
  delete request_context_;
  delete response_context_;
}

void HttpCodecService::OnDataFinishSend(const SocketChannel* channel) {
}

void HttpCodecService::OnDataReceived(const SocketChannel* channel, IOBuffer *buf) {
  DCHECK(channel == channel_.get());

  VLOG(GLOG_VTRACE) << __FUNCTION__ << " buffer_size:" << buf->CanReadSize();

  bool success = IsServerSide() ?
    ParseHttpRequest(channel_.get(), buf) : ParseHttpResponse(channel_.get(), buf);

  if (!success) {
  	CloseService();
  }
}

bool HttpCodecService::ParseHttpRequest(TcpChannel* channel, IOBuffer* buf) {
  size_t buffer_size = buf->CanReadSize();
  const char* buffer_start = (const char*)buf->GetRead();
  http_parser* parser = request_context_->Parser();


  size_t nparsed = http_parser_execute(parser,
                                       &req_parser_settings_,
                                       buffer_start,
                                       buffer_size);

  VLOG(GLOG_VTRACE) << __FUNCTION__ << " http_parser_execute consumed size:" << nparsed;
  buf->Consume(nparsed);

  if (parser->upgrade) {
    LOG(ERROR) << " Not Supported Now";

    request_context_->current_.reset();
    channel->Send(HttpConstant::kBadRequest.data(), HttpConstant::kBadRequest.size());

    return false;
  } else if (nparsed != buffer_size) {
    LOG(ERROR) << " Parse Occur ERROR, nparsed" << nparsed << " buffer_size:" << buffer_size;

    request_context_->current_.reset();
    channel->Send(HttpConstant::kBadRequest.data(), HttpConstant::kBadRequest.size());

    return false;
  }

  if (!delegate_) {
    LOG(ERROR) << "No message handler for this http protocol service message, shutdown this channel";
    return false;
  }

  while (request_context_->messages_.size()) {
    RefHttpRequest message = request_context_->messages_.front();
    request_context_->messages_.pop_front();

    message->SetIOCtx(shared_from_this());
    CHECK(message->GetMessageType() == MessageType::kRequest);
    VLOG(GLOG_VTRACE) << " pass message to message reciever" << channel_->ChannelInfo();
    delegate_->OnCodecMessage(RefCast(CodecMessage, message));
  }
  return true;
}

bool HttpCodecService::ParseHttpResponse(TcpChannel* channel, IOBuffer* buf) {
  VLOG(GLOG_VTRACE) << __FUNCTION__ << " enter";

  size_t buffer_size = buf->CanReadSize();
  const char* buffer_start = (const char*)buf->GetRead();
  http_parser* parser = response_context_->Parser();
  size_t nparsed = http_parser_execute(parser,
                                       &res_parser_settings_,
                                       buffer_start,
                                       buffer_size);

  buf->Consume(nparsed);
  if (parser->upgrade) { //websockt
    LOG(ERROR) << " Not Supported Now";
    response_context_->current_.reset();
    return false;
  } else if (nparsed != buffer_size) {
    LOG(ERROR) << " Parse Occur ERROR";
    response_context_->current_.reset();
    return false;
  }
  if (!delegate_) {
    LOG(ERROR) << __FUNCTION__ << " no message handler message, ";
    return false;
  }

  while (!response_context_->messages_.empty()) {
    RefHttpResponse message = response_context_->messages_.front();
    response_context_->messages_.pop_front();

    message->SetIOCtx(shared_from_this());
    CHECK(message->GetMessageType() == MessageType::kResponse);

    delegate_->OnCodecMessage(RefCast(CodecMessage, message));
  }
  return true;
}

//static
bool HttpCodecService::RequestToBuffer(const HttpRequest* request, IOBuffer* buffer) {
  CHECK(request && buffer);
  int32_t guess_size = kHttpMsgReserveSize + request->Body().size();
  guess_size += request->Headers().size() * kMeanHeaderSize;
  buffer->EnsureWritableSize(guess_size);

  buffer->WriteString(request->Method());
  buffer->WriteString(HttpConstant::kBlankSpace);
  buffer->WriteString(request->RequestUrl());
  buffer->WriteString(HttpConstant::kBlankSpace);

  char v[11]; //"HTTP/1.1\r\n"
  snprintf(v, 11, "HTTP/1.%d\r\n", request->VersionMinor() == 1 ? 1 : 0);
  buffer->WriteRawData(v, 10);

  for (const auto& header : request->Headers()) {
    buffer->WriteString(header.first);
    buffer->WriteRawData(": ", 2);
    buffer->WriteString(header.second);
    buffer->WriteString(HttpConstant::kCRCN);
  }

  if (!request->HasHeaderField(HttpConstant::kConnection)) {
    buffer->WriteString(request->IsKeepAlive() ?
                        HttpConstant::kHeaderKeepAlive :
                        HttpConstant::kHeaderNotKeepAlive);
  }

  if (!request->HasHeaderField(HttpConstant::kAcceptEncoding)) {
    buffer->WriteString(HttpConstant::kHeaderSupportedEncoding);
  }

  if (!request->HasHeaderField(HttpConstant::kContentLength)) {
    buffer->WriteString(HttpConstant::kContentLength);
    std::string content_len(": ");
    content_len.append(std::to_string(request->Body().size()));
    content_len.append(HttpConstant::kCRCN);
    buffer->WriteString(content_len);
  }

  if (!request->HasHeaderField(HttpConstant::kContentType)) {
    buffer->WriteString(HttpConstant::kHeaderDefaultContentType);
  }
  buffer->WriteString(HttpConstant::kCRCN);

  buffer->WriteString(request->Body());
  return true;
}

bool HttpCodecService::SendRequest(CodecMessage* message) {
	CHECK(message->GetMessageType() == MessageType::kRequest);

  auto request = static_cast<HttpRequest*>(message);
  BeforeSendRequest(request);

  if (!RequestToBuffer(request, channel_->WriterBuffer())) {
    return false;
  }
  return channel_->TryFlush();
}

bool HttpCodecService::SendResponse(const CodecMessage* req, CodecMessage* res) {
  HttpResponse* response = static_cast<HttpResponse*>(res);
  const HttpRequest* request = static_cast<const HttpRequest*>(req);

  BeforeSendResponseMessage(request, response);

  if (!ResponseToBuffer(response, channel_->WriterBuffer())) {
    LOG(ERROR) << __FUNCTION__ << " failed encode:" << response->Dump();
    return false;
  }
  VLOG(GLOG_VINFO) << __FUNCTION__ << " write response:" << response->Dump();
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
    channel_->ShutdownChannel(true);
  }
  return channel_->TryFlush();
};

//static
bool HttpCodecService::ResponseToBuffer(const HttpResponse* response, IOBuffer* buffer) {
  CHECK(response && buffer);

  int32_t guess_size = kHttpMsgReserveSize + response->Headers().size() * kMeanHeaderSize + response->Body().size();
  buffer->EnsureWritableSize(guess_size);

  std::ostringstream oss;
  int32_t code = response->ResponseCode();
  oss << "HTTP/1." << response->VersionMinor() << " " << code << " "
      << HttpConstant::StatusCodeCStr(code) << HttpConstant::kCRCN;
  buffer->WriteString(oss.str());

  for (const auto& header : response->Headers()) {
    buffer->WriteString(header.first);
    buffer->WriteRawData(": ", 2);
    buffer->WriteString(header.second);
    buffer->WriteString(HttpConstant::kCRCN);
  }

  if (!response->HasHeaderField(HttpConstant::kConnection)) {
    buffer->WriteString(response->IsKeepAlive() ?
                        HttpConstant::kHeaderKeepAlive :
                        HttpConstant::kHeaderNotKeepAlive);
  }

  if (!response->HasHeaderField(HttpConstant::kContentLength)) {
    std::string content_len(HttpConstant::kContentLength);
    content_len.append(": ");
    content_len.append(std::to_string(response->Body().size()));
    content_len.append(HttpConstant::kCRCN);
    buffer->WriteString(content_len);
  }

  if (!response->HasHeaderField(HttpConstant::kContentType)) {
    buffer->WriteString(HttpConstant::kHeaderDefaultContentType);
  }
  buffer->WriteString(HttpConstant::kCRCN);

  buffer->WriteString(response->Body());
  return true;
}

const RefCodecMessage HttpCodecService::NewResponse(const CodecMessage* request)  {
  CHECK(request && request->GetMessageType() == MessageType::kRequest);
  const HttpRequest* http_request = (HttpRequest*)request;//static_cast<HttpRequest*>(request);

  RefHttpResponse http_res = HttpResponse::CreateWithCode(500);

  http_res->SetKeepAlive(http_request->IsKeepAlive());
  http_res->InsertHeader("Content-Type", "text/plain");
  return std::move(http_res);
}

void HttpCodecService::BeforeSendRequest(HttpRequest* out_message) {

  HttpRequest* request = static_cast<HttpRequest*>(out_message);
  if (request->Body().size() > kCompressionThreshold &&
      !request->HasHeaderField(HttpConstant::kContentEncoding)) {

    std::string compressed_body;
    if (0 == base::Gzip::compress_gzip(request->Body(), compressed_body)) { //success
      request->InsertHeader(HttpConstant::kContentEncoding, "gzip");
      request->InsertHeader(HttpConstant::kContentLength, std::to_string(compressed_body.size()));
      request->body_ = std::move(compressed_body);
    }
  }

  if (!out_message->HasHeaderField(HttpConstant::kHost)) {
    request->InsertHeader(HttpConstant::kHost, request->RemoteHost());
  }
}

bool HttpCodecService::BeforeSendResponseMessage(const HttpRequest* request, HttpResponse* response) {
  //response compression if needed
  if (response->Body().size() > kCompressionThreshold &&
      !response->HasHeaderField(HttpConstant::kContentEncoding)) {

    const std::string& accept = request->GetHeader(HttpConstant::kAcceptEncoding);
    std::string compressed_body;
    if (accept.find("gzip") != std::string::npos) {
      if (0 == base::Gzip::compress_gzip(response->Body(), compressed_body)) { //success
        response->body_ = std::move(compressed_body);
        response->InsertHeader("Content-Encoding", "gzip");
      }
    } else if (accept.find("deflate") != std::string::npos) {
      if (0 == base::Gzip::compress_deflate(response->Body(), compressed_body)) {
        response->body_ = std::move(compressed_body);
        response->InsertHeader("Content-Encoding", "deflate");
      }
    }
  }
  return true;
}

}}//end namespace
