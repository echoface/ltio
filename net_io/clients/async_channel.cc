#include "async_channel.h"
#include "base/base_constants.h"
#include "protocol/proto_service.h"

namespace lt {
namespace net {

RefAsyncChannel AsyncChannel::Create(Delegate* d, RefProtoService& s) {
  return RefAsyncChannel(new AsyncChannel(d, s));
}

AsyncChannel::AsyncChannel(Delegate* d, RefProtoService& s)
  : ClientChannel(d, s) {

}

AsyncChannel::~AsyncChannel() {
}

void AsyncChannel::StartClient() {
  //common part
  ClientChannel::StartClient();
}

void AsyncChannel::SendRequest(RefProtocolMessage request)  {
  CHECK(IOLoop()->IsInLoopThread());
  // guard herer ?
  // A -> b -> c -> call something it delete this Channel --> Back to A
  request->SetIOCtx(protocol_service_);

  bool success = protocol_service_->SendRequestMessage(request);
  if (!success) {
    request->SetFailCode(MessageCode::kConnBroken);
    delegate_->OnRequestGetResponse(request, ProtocolMessage::kNullMessage);
    return;
  }

  WeakProtocolMessage weak(request); // weak ptr must init outside, Take Care of weakptr
  auto functor = std::bind(&AsyncChannel::OnRequestTimeout, shared_from_this(), weak);
  IOLoop()->PostDelayTask(NewClosure(functor), request_timeout_);

  uint64_t message_identify = request->AsyncIdentify();

  CHECK(message_identify != SequenceIdentify::KNullId);
  in_progress_.insert(std::make_pair(message_identify, std::move(request)));
}

void AsyncChannel::OnRequestTimeout(WeakProtocolMessage weak) {
  RefProtocolMessage request = weak.lock();
  if (!request || request->IsResponded()) {
    return;
  }

  uint64_t identify = request->AsyncIdentify();
  CHECK(identify != SequenceIdentify::KNullId);

  size_t numbers = in_progress_.erase(identify);
  CHECK(numbers == 1);

  request->SetFailCode(MessageCode::kTimeOut);
  delegate_->OnRequestGetResponse(request, ProtocolMessage::kNullMessage);
}

void AsyncChannel::OnProtocolMessage(const RefProtocolMessage& res) {
  DCHECK(IOLoop()->IsInLoopThread());

  uint64_t identify = res->AsyncIdentify();
  CHECK(identify != SequenceIdentify::KNullId);


  auto iter = in_progress_.find(identify);
  if (iter == in_progress_.end()) {
    VLOG(GLOG_VINFO) << __FUNCTION__ << " response:" << identify << " not found corresponding request";
    return;
  }

  CHECK(iter->second->AsyncIdentify() == identify);

  delegate_->OnRequestGetResponse(iter->second, res);
  in_progress_.erase(iter);
}

void AsyncChannel::OnProtocolServiceGone(const RefProtoService& service) {
	DCHECK(IOLoop()->IsInLoopThread());

  for (auto kv : in_progress_) {
    kv.second->SetFailCode(MessageCode::kConnBroken);
    delegate_->OnRequestGetResponse(kv.second, ProtocolMessage::kNullMessage);
  }
  in_progress_.clear();
  auto guard = shared_from_this();
  delegate_->OnClientChannelClosed(guard);
}


}}//lt::net

