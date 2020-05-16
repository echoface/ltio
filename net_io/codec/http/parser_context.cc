#include "parser_context.h"
#include "base/base_constants.h"
#include "http_constants.h"

#include "glog/logging.h"
#include <base/utils/gzip/gzip_utils.h>
#include <net_io/codec/codec_message.h>
#include <thirdparty/http_parser/http_parser.h>

namespace lt {
namespace net {

ResParseContext::ResParseContext()
  : last_is_header_value(false) {

  http_parser_init(&parser_, HTTP_RESPONSE);
  parser_.data = this;
}
void ResParseContext::reset() {
  current_.reset();
  half_header.first.clear();
  half_header.second.clear();
  last_is_header_value = false;
}

http_parser* ResParseContext::Parser() {
  return &parser_;
}

ReqParseContext::ReqParseContext()
  : last_is_header_value(false) {

  http_parser_init(&parser_, HTTP_REQUEST);
  parser_.data = this;
}

void ReqParseContext::reset() {
  current_.reset();
  half_header.first.clear();
  half_header.second.clear();
  last_is_header_value = false;
}

http_parser* ReqParseContext::Parser() {
  return &parser_;
}

//static
int ReqParseContext::OnHttpRequestBegin(http_parser* parser) {
  ReqParseContext* context = (ReqParseContext*)parser->data;
  CHECK(context);
  VLOG(GLOG_VTRACE) << __FUNCTION__ << "enter";

  if (context->current_) {
    context->reset();
    LOG(ERROR) << "Something Wrong, Current Should Be Null";
  }
  context->current_ = std::make_shared<HttpRequest>();

  context->current_->url_.clear();

  return 0;
}

int ReqParseContext::OnUrlParsed(http_parser* parser, const char *url_start, size_t url_len) {
  ReqParseContext* context = (ReqParseContext*)parser->data;
  CHECK(context);

  VLOG(GLOG_VTRACE) << __FUNCTION__ << "on url parsed";
  context->current_->url_.append(url_start, url_len);
  return 0;
}

int ReqParseContext::OnStatusCodeParsed(http_parser* parser, const char *start, size_t len) {
  VLOG(GLOG_VTRACE) << __FUNCTION__ << "request should not reached here";
  return 0;
}

int ReqParseContext::OnHeaderFieldParsed(http_parser* parser, const char *header_start, size_t len) {
  ReqParseContext* context = (ReqParseContext*)parser->data;
  CHECK(context);
  VLOG(GLOG_VTRACE) << __FUNCTION__ << "enter";

  if (context->last_is_header_value) {
    if (!context->half_header.first.empty()) {
      context->current_->InsertHeader(context->half_header.first, context->half_header.second);
    }
    context->half_header.first.clear();
    context->half_header.second.clear();
  }
  context->last_is_header_value = false;
  context->half_header.first.append(header_start, len);
  return 0;
}

int ReqParseContext::OnHeaderValueParsed(http_parser* parser, const char *value_start, size_t len) {
  ReqParseContext* context = (ReqParseContext*)parser->data;
  CHECK(context);

  context->last_is_header_value = true;
  if (context->half_header.first.empty()) {
    LOG(ERROR) << __FUNCTION__ << " Got A Empty HeaderFiled";
    return 0;
  }
  VLOG(GLOG_VTRACE) << __FUNCTION__ << " extract header value";
  context->half_header.second.append(value_start, len);
  return 0;
}

int ReqParseContext::OnHeaderFinishParsed(http_parser* parser) {
  ReqParseContext* context = (ReqParseContext*)parser->data;
  CHECK(context);

  VLOG(GLOG_VTRACE) << __FUNCTION__ << " enter";

  if (!context->half_header.first.empty()) {
    context->current_->InsertHeader(context->half_header.first, context->half_header.second);
  }
  context->half_header.first.clear();
  context->half_header.second.clear();
  context->last_is_header_value = false;

  return 0;
}

int ReqParseContext::OnBodyParsed(http_parser* parser, const char *body_start, size_t len) {
  ReqParseContext* context = (ReqParseContext*)parser->data;
  VLOG(GLOG_VTRACE) << __FUNCTION__ << " enter";

  context->current_->MutableBody().append(body_start, len);

  return 0;
}

int ReqParseContext::OnHttpRequestEnd(http_parser* parser) {
  ReqParseContext* context = (ReqParseContext*)parser->data;

  VLOG(GLOG_VTRACE) << __FUNCTION__ << " enter";

  int type = parser->type;
  if (type != HTTP_REQUEST) {
    LOG(ERROR) << __FUNCTION__ << "ReqParseContext Parse A NONE-REQUEST Content";
    context->reset();
    return -1;
  }

  //context->current_->SetMessageType(MessagetType::kRequest);
  context->current_->keepalive_ = http_should_keep_alive(parser);
  context->current_->http_major_ = parser->http_major;
  context->current_->http_minor_ = parser->http_minor;

  context->current_->method_ = http_method_str((http_method)parser->method);
  //gzip, inflat

  const std::string kgzip("gzip");
  const std::string& encoding = context->current_->GetHeader(HttpConstant::kContentEncoding);;
  if (encoding.find(kgzip) != std::string::npos) {
    std::string decompress_body;
    if (0 != base::Gzip::decompress_gzip(context->current_->body_, decompress_body)) {
      LOG(ERROR) << " Decode gzip HttpRequest Body Failed";
      context->current_->body_.clear();
    }
    context->current_->body_ = std::move(decompress_body);
  } else if (encoding.find("deflate") != std::string::npos){
    std::string decompress_body;
    if (0 != base::Gzip::decompress_deflate(context->current_->body_, decompress_body)) {
      LOG(ERROR) << " Decode deflate HttpRequest Body Failed";
      context->current_->body_.clear();
    }
    context->current_->body_ = std::move(decompress_body);
  }
  //build params
  //extract host

  VLOG(GLOG_VTRACE) << __FUNCTION__ << " new message born";
  context->messages_.push_back(std::move(context->current_));

  context->current_.reset();

  return 0;
}

int ReqParseContext::OnChunkHeader(http_parser* parser) {
  return 0;
}
int ReqParseContext::OnChunkFinished(http_parser* parser) {
  return 0;
}


// response

int ResParseContext::OnHttpResponseBegin(http_parser* parser) {
  ResParseContext* context = (ResParseContext*)parser->data;
  CHECK(context);

  if (context->current_) {
    LOG(ERROR) << "Something Wrong, Current Should Be Null";
  }
  context->current_ = std::make_shared<HttpResponse>();
  return 0;
}
int ResParseContext::OnUrlParsed(http_parser* parser, const char *url_start, size_t url_len) {
  return 0;
}
int ResParseContext::OnStatusCodeParsed(http_parser* parser, const char *start, size_t len) {
  ResParseContext* context = (ResParseContext*)parser->data;
  CHECK(context);
  return 0;
}

int ResParseContext::OnHeaderFieldParsed(http_parser* parser, const char *header_start, size_t len) {
  ResParseContext* context = (ResParseContext*)parser->data;
  CHECK(context);

  if (context->last_is_header_value) {
    if (!context->half_header.first.empty()) {
      context->current_->InsertHeader(context->half_header.first, context->half_header.second);
    }
    context->half_header.first.clear();
    context->half_header.second.clear();
  }
  context->last_is_header_value = false;
  context->half_header.first.append(header_start, len);

  return 0;
}

int ResParseContext::OnHeaderValueParsed(http_parser* parser, const char *value_start, size_t len) {
  ResParseContext* context = (ResParseContext*)parser->data;
  CHECK(context);

  context->last_is_header_value = true;
  if (context->half_header.first.empty()) {
    LOG(ERROR) << __FUNCTION__ << " Got A Empty HeaderFiled";
    return 0;
  }
  context->half_header.second.append(value_start, len);
  return 0;
}

int ResParseContext::OnHeaderFinishParsed(http_parser* parser) {
  ResParseContext* context = (ResParseContext*)parser->data;
  CHECK(context);

  if (!context->half_header.first.empty()) {
    context->current_->InsertHeader(context->half_header.first, context->half_header.second);
  }
  context->half_header.first.clear();
  context->half_header.second.clear();
  context->last_is_header_value = false;

  return 0;
}

int ResParseContext::OnBodyParsed(http_parser* parser, const char *body_start, size_t len) {
  ResParseContext* context = (ResParseContext*)parser->data;
  CHECK(context);

  context->current_->body_.append(body_start, len);
  return 0;
}
int ResParseContext::OnHttpResponseEnd(http_parser* parser) {
  ResParseContext* context = (ResParseContext*)parser->data;
  CHECK(context);

  CHECK(parser->type == HTTP_RESPONSE);
  context->current_->keepalive_ = http_should_keep_alive(parser);
  context->current_->http_major_ = parser->http_major;
  context->current_->http_minor_ = parser->http_minor;
  context->current_->status_code_ = parser->status_code;

  const std::string& encoding = context->current_->GetHeader(HttpConstant::kContentEncoding);;
  if (encoding.find("gzip") != std::string::npos) {
    std::string decompress_body;
    if (0 != base::Gzip::decompress_gzip(context->current_->body_, decompress_body)) {
      LOG(ERROR) << " Decode gzip HttpRequest Body Failed";
    }
    context->current_->body_ = std::move(decompress_body);
  }
  else if (encoding.find("deflate") != std::string::npos) {
    std::string decompress_body;
    if (0 != base::Gzip::decompress_deflate(context->current_->body_, decompress_body)) {
      LOG(ERROR) << " Decode deflate HttpRequest Body Failed";
    }
    context->current_->body_ = std::move(decompress_body);
  }

  context->messages_.push_back(std::move(context->current_));
  context->current_.reset();
  return 0;
}

int ResParseContext::OnChunkHeader(http_parser* parser) {
  return 0;
}
int ResParseContext::OnChunkFinished(http_parser* parser) {
  return 0;
}

}}
