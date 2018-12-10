#include "async_channel.h"
#include "base/base_constants.h"
#include "protocol/proto_service.h"

namespace net {

const static RefProtocolMessage kNullResponse;

AsyncChannel::AsyncChannel(Delegate* d, RefProtoService& s)
  : ClientChannel(d, s) {
}

AsyncChannel::~AsyncChannel() {
}

void AsyncChannel::StartClient() {

  ProtoMessageHandler res_handler =
      std::bind(&AsyncChannel::OnResponseMessage,this, std::placeholders::_1);
  protocol_service_->SetMessageHandler(std::move(res_handler));
  protocol_service_->Channel()->Start();
}

void AsyncChannel::SendRequest(RefProtocolMessage request)  {
  CHECK(IOLoop()->IsInLoopThread());

  request->SetIOContext(protocol_service_);
  bool success = protocol_service_->SendRequestMessage(request);
  if (!success) {
    request->SetFailInfo(FailInfo::kChannelBroken);
    delegate_->OnRequestGetResponse(request, kNullResponse);
    return;
  }

  WeakProtocolMessage weak(request); // weak ptr must init outside, Take Care of weakptr
  auto functor = std::bind(&AsyncChannel::OnRequestTimeout, shared_from_this(), weak);
  IOLoop()->PostDelayTask(NewClosure(functor), request_timeout_);

  uint64_t message_identify = request->MessageIdentify();
  CHECK(message_identify != MessageIdentifyType::KInvalidIdentify);
  in_progress_.insert(std::make_pair(message_identify, std::move(request)));
}

void AsyncChannel::OnRequestTimeout(WeakProtocolMessage weak) {

  RefProtocolMessage request = weak.lock();
  if (!request || request->IsResponsed()) {
    return;
  }

  uint64_t identify = request->MessageIdentify();
  CHECK(identify != MessageIdentifyType::KInvalidIdentify);

  size_t numbers = in_progress_.erase(identify);
  CHECK(numbers == 1);

  request->SetFailInfo(FailInfo::kTimeOut);
  delegate_->OnRequestGetResponse(request, kNullResponse);
}

void AsyncChannel::OnResponseMessage(const RefProtocolMessage& res) {
  //CHECK(IOLoop()->IsInLoopThread());

  uint64_t identify = res->MessageIdentify();
  CHECK(identify != MessageIdentifyType::KInvalidIdentify);

  auto iter = in_progress_.find(identify);
  if (iter == in_progress_.end()) {
    return;
  }

  CHECK(iter->second->MessageIdentify() == identify);

  delegate_->OnRequestGetResponse(iter->second, res);
  in_progress_.erase(iter);
}

void AsyncChannel::OnProtocolServiceGone(const RefProtoService& service) {
  //CHECK(IOLoop()->IsInLoopThread());
  for (auto kv : in_progress_) {
    kv.second->SetFailInfo(FailInfo::kChannelBroken);
    delegate_->OnRequestGetResponse(kv.second, kNullResponse);
  }
  in_progress_.clear();
  auto guard = shared_from_this();
  delegate_->OnClientChannelClosed(guard);
}

}//net

