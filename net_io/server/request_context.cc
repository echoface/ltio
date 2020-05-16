
#include "request_context.h"

namespace lt {
namespace net {

CommonRequestContext::CommonRequestContext(const RefCodecMessage&& req)
  : request_(req) {
};

CommonRequestContext::~CommonRequestContext() {
  if (!IsResponded()) {
    auto service = request_->GetIOCtx().codec.lock();
    if (!service) {
      return;
    }
    auto response = service->NewResponse(request_.get());
    do_response(response);
  }
}

void CommonRequestContext::Send(const RefCodecMessage& response) {
  if (done_) return;
  return do_response(response);
}

void CommonRequestContext::do_response(const RefCodecMessage& response) {
  done_ = true;
  auto service = request_->GetIOCtx().codec.lock();
  LOG_IF(ERROR, !service) << __FUNCTION__ << " connection has broken";
  if (!service) { return;}

  if (service->IOLoop()->IsInLoopThread()) {
    if (!service->EncodeResponseToChannel(request_.get(), response.get())) {
      service->CloseService();
    }
    return;
  }
  service->IOLoop()->PostTask(FROM_HERE, [=]() {
    if (!service->EncodeResponseToChannel(request_.get(), response.get())) {
      service->CloseService();
    }
  });
}

}}
