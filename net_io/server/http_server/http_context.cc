#include <glog/logging.h>
#include "http_context.h"
#include "protocol/proto_service.h"

namespace lt {
namespace net {

HttpContext::HttpContext(const RefHttpRequest& request)
  : request_(request) {
  io_loop_ = request->GetIOCtx().io_loop;
}

void HttpContext::ReplyFile(const std::string& path, uint16_t code) {
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

  auto service = request_->GetIOCtx().protocol_service.lock();
  if (!service) {
    LOG(ERROR) << __FUNCTION__ << " Connection Has Broken";
    return;
  }

  bool keep_alive = request_->IsKeepAlive();
  response->SetKeepAlive(keep_alive);

  if (!io_loop_->IsInLoopThread()) {
    auto req = request_;

    auto functor = [=]() {
      bool success = service->SendResponseMessage(RefCast(ProtocolMessage, req),
                                                  RefCast(ProtocolMessage, response));
      if (!keep_alive || !success) {
        service->CloseService();
      }
    };
    io_loop_->PostTask(NewClosure(std::move(functor)));

    return;
  }

  bool success = service->SendResponseMessage(RefCast(ProtocolMessage, request_),
                                              RefCast(ProtocolMessage, response));
  if (!keep_alive || !success) {
    service->CloseService();
  }
}

}}
