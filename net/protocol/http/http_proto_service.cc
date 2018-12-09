
#include "glog/logging.h"

#include "tcp_channel.h"
#include "net/io_buffer.h"
#include "http_proto_service.h"
#include "http_constants.h"
#include "net/protocol/proto_message.h"
#include "base/utils/gzip/gzip_utils.h"

namespace net {

static const int32_t kMeanHeaderSize = 32;
static const int32_t kHttpMsgReserveSize = 64;
static const int32_t kCompressionThreshold = 4096;

//static
http_parser_settings HttpProtoService::req_parser_settings_ = {
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

http_parser_settings HttpProtoService::res_parser_settings_ = {
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

HttpProtoService::HttpProtoService()
  : ProtoService(),
    request_context_(NULL),
    response_context_(NULL) {

  request_context_ = new ReqParseContext();
  response_context_ = new ResParseContext();
}

HttpProtoService::~HttpProtoService() {
  delete request_context_;
  delete response_context_;
}

void HttpProtoService::OnStatusChanged(const RefTcpChannel& channel) {
}

void HttpProtoService::OnDataFinishSend(const RefTcpChannel& channel) {
}

void HttpProtoService::OnDataRecieved(const RefTcpChannel& channel, IOBuffer* buf) {;
  bool success = IsServerSideservice() ? ParseHttpRequest(channel, buf) : ParseHttpResponse(channel, buf);
  if (!success) {
  	CloseService();
  }
}

bool HttpProtoService::DecodeToMessage(IOBuffer* buffer, ProtocolMessage* out_msg) {
  return false;
}

bool HttpProtoService::EncodeToBuffer(const ProtocolMessage* msg, IOBuffer* out_buffer) {
  CHECK(msg && out_buffer);

  switch (msg->GetMessageType()) {
    case MessageType::kRequest: {
      const HttpRequest* request = static_cast<const HttpRequest*>(msg);
      return RequestToBuffer(request, out_buffer);
    } break;
    case MessageType::kResponse: {
      const HttpResponse* response = static_cast<const HttpResponse*>(msg);
      return ResponseToBuffer(response, out_buffer);
    } break;
    default:
      LOG(ERROR) << __FUNCTION__ << " Should Not Readched";
    break;
  }
  return false;
}

bool HttpProtoService::ParseHttpRequest(const RefTcpChannel& channel, IOBuffer* buf) {
  size_t buffer_size = buf->CanReadSize();
  const char* buffer_start = (const char*)buf->GetRead();
  http_parser* parser = request_context_->Parser();


  size_t nparsed = http_parser_execute(parser,
                                       &req_parser_settings_,
                                       buffer_start,
                                       buffer_size);

  buf->Consume(nparsed);

  if (parser->upgrade) {
    LOG(ERROR) << " Not Supported Now";

    request_context_->current_.reset();
    channel->Send((const uint8_t*)HttpConstant::kBadRequest.data(), HttpConstant::kBadRequest.size());

    return false;
  } else if (nparsed != buffer_size) {
    LOG(ERROR) << " Parse Occur ERROR, nparsed" << nparsed
               << " buffer_size:" << buffer_size;

    request_context_->current_.reset();
    channel->Send((const uint8_t*)HttpConstant::kBadRequest.data(), HttpConstant::kBadRequest.size());

    return false;
  }

  if (!message_handler_) {
    LOG(ERROR) << "No message handler for this http protocol service message, shutdown this channel";
    return false;
  }

  while (request_context_->messages_.size()) {
    RefHttpRequest message = request_context_->messages_.front();
    request_context_->messages_.pop_front();

    message->SetIOContext(shared_from_this());
    CHECK(message->GetMessageType() == MessageType::kRequest);
    message_handler_(std::static_pointer_cast<ProtocolMessage>(message));
  }
  return true;
}

bool HttpProtoService::ParseHttpResponse(const RefTcpChannel& channel, IOBuffer* buf) {

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
  if (!message_handler_) {
    LOG(ERROR) << __FUNCTION__ << " no message handler message, ";
    return false;
  }

  while (response_context_->messages_.size()) {
    RefHttpResponse message = response_context_->messages_.front();
    response_context_->messages_.pop_front();

    message->SetIOContext(shared_from_this());
    CHECK(message->GetMessageType() == MessageType::kResponse);

    if (message_handler_) {
      message_handler_(std::static_pointer_cast<ProtocolMessage>(message));
    }
  }
  return true;
}

//static
bool HttpProtoService::RequestToBuffer(const HttpRequest* request, IOBuffer* buffer) {
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

bool HttpProtoService::SendProtocolMessage(RefProtocolMessage& message) {
  BeforeWriteMessage(message.get());

  IOBuffer buffer;
  if (!EncodeToBuffer(message.get(), &buffer)) {
    return false;
  }
  return channel_->Send(buffer.GetRead(), buffer.CanReadSize()) >= 0;
}

//static
bool HttpProtoService::ResponseToBuffer(const HttpResponse* response, IOBuffer* buffer) {
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

const RefProtocolMessage HttpProtoService::NewResponseFromRequest(const RefProtocolMessage& request)  {
  CHECK(request && request->GetMessageType() == MessageType::kRequest);

  const HttpRequest* http_request = static_cast<HttpRequest*>(request.get());

  RefHttpResponse http_res = HttpResponse::CreatWithCode(500);

  http_res->SetKeepAlive(http_request->IsKeepAlive());
  http_res->InsertHeader("Content-Type", "text/plain");

  return std::move(http_res);
}

bool HttpProtoService::CloseAfterMessage(ProtocolMessage* request, ProtocolMessage* response) {
  if (!request) {return true;}

  return !static_cast<HttpRequest*>(request)->IsKeepAlive();
}

void HttpProtoService::BeforeWriteRequestToBuffer(ProtocolMessage* out_message) {
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
}

void HttpProtoService::BeforeWriteResponseToBuffer(ProtocolMessage* out_message) {
  HttpResponse* response = static_cast<HttpResponse*>(out_message);
  if (response->Body().size() > kCompressionThreshold &&
      !response->HasHeaderField(HttpConstant::kContentEncoding)) {

    std::string compressed_body;
    if (0 == base::Gzip::compress_gzip(response->Body(), compressed_body)) { //success
      response->InsertHeader(HttpConstant::kContentEncoding, "gzip");
      response->InsertHeader(HttpConstant::kContentLength, std::to_string(compressed_body.size()));
      response->body_ = std::move(compressed_body);
    }
  }
}

void HttpProtoService::BeforeSendResponse(ProtocolMessage *in, ProtocolMessage *out) {
  HttpRequest* request = static_cast<HttpRequest*>(in);
  HttpResponse* response = static_cast<HttpResponse*>(out);

  //response compression if needed
  if (response->Body().size() > 4096 &&
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
}

}//end namespace
