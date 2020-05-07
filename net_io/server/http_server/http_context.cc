
#include <glog/logging.h>
#include "net_io/codec/codec_service.h"
#include "base/message_loop/message_loop.h"

#include "http_context.h"

namespace lt {
namespace net {

HttpContext::HttpContext(const RefHttpRequest& request)
  : request_(request) {
  io_loop_ = request->GetIOCtx().io_loop;
}

void HttpContext::ReplyFile(const std::string& path, uint16_t code) {
  CHECK(false);
  if (did_reply_) return;
}

void HttpContext::ReplyJson(const std::string& json, uint16_t code) {
  if (did_reply_) return;

  RefHttpResponse response = HttpResponse::CreatWithCode(code);
  response->InsertHeader("Content-Type", "application/json;utf-8");
  response->MutableBody() = json;

  DoReplyResponse(response);
}

void HttpContext::ReplyString(const char* content, uint16_t code) {
  if (did_reply_) return;

  RefHttpResponse response = HttpResponse::CreatWithCode(code);
  response->MutableBody().append(content);

  DoReplyResponse(response);
}

void HttpContext::ReplyString(const std::string& content, uint16_t code) {
  if (did_reply_) return;

  RefHttpResponse response = HttpResponse::CreatWithCode(code);
  response->MutableBody() = std::move(content);

  DoReplyResponse(response);
}

void HttpContext::DoReplyResponse(RefHttpResponse& response) {
  did_reply_ = true;

  auto service = request_->GetIOCtx().codec.lock();
  if (!service) {
    LOG(ERROR) << __FUNCTION__ << " Connection Has Broken";
    return;
  }

  bool keep_alive = request_->IsKeepAlive();
  response->SetKeepAlive(keep_alive);

  if (!io_loop_->IsInLoopThread()) {
    auto req = request_;

    auto functor = [=]() {
      bool success = service->EncodeResponseToChannel(req.get(), response.get());
      if (!keep_alive || !success) {
        service->CloseService();
      }
    };
    io_loop_->PostTask(NewClosure(std::move(functor)));
    return;
  }

  bool success = service->EncodeResponseToChannel(request_.get(), response.get());
  if (!keep_alive || !success) {
    service->CloseService();
  }
}

}}
