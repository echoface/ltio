#include "async_channel.h"
#include <base/base_constants.h>
#include <net_io/codec/codec_service.h>

namespace lt {
namespace net {

RefAsyncChannel AsyncChannel::Create(Delegate* d, const RefCodecService& s) {
  return RefAsyncChannel(new AsyncChannel(d, s));
}

AsyncChannel::AsyncChannel(Delegate* d, const RefCodecService& s)
  : ClientChannel(d, s) {
}

AsyncChannel::~AsyncChannel() {
}

void AsyncChannel::StartClientChannel() {
  //common part
  ClientChannel::StartClientChannel();
}

void AsyncChannel::SendRequest(RefCodecMessage request)  {
  CHECK(IOLoop()->IsInLoopThread());
  // guard herer ?
  // A -> b -> c -> call something it delete this Channel --> Back to A
  request->SetIOCtx(codec_);

  bool success = codec_->EncodeToChannel(request.get());
  if (!success) {
    request->SetFailCode(MessageCode::kConnBroken);
    HandleResponse(request, CodecMessage::kNullMessage);
    return;
  }

  WeakCodecMessage weak(request); // weak ptr must init outside, Take Care of weakptr
  auto functor = std::bind(&AsyncChannel::OnRequestTimeout, shared_from_this(), weak);
  IOLoop()->PostDelayTask(NewClosure(functor), request_timeout_);

  uint64_t message_identify = request->AsyncId();
  in_progress_.insert(std::make_pair(message_identify, std::move(request)));
}

void AsyncChannel::OnRequestTimeout(WeakCodecMessage weak) {
  RefCodecMessage request = weak.lock();
  if (!request || request->IsResponded()) {
    return;
  }

  uint64_t identify = request->AsyncId();

  size_t numbers = in_progress_.erase(identify);
  CHECK(numbers == 1);

  request->SetFailCode(MessageCode::kTimeOut);
  HandleResponse(request, CodecMessage::kNullMessage);
}

void AsyncChannel::BeforeCloseChannel() {
  DCHECK(IOLoop()->IsInLoopThread());

  for (auto kv : in_progress_) {
    kv.second->SetFailCode(MessageCode::kConnBroken);
    HandleResponse(kv.second, CodecMessage::kNullMessage);
  }
  in_progress_.clear();
}

void AsyncChannel::OnCodecMessage(const RefCodecMessage& res) {
  DCHECK(IOLoop()->IsInLoopThread());

  uint64_t identify = res->AsyncId();

  auto iter = in_progress_.find(identify);
  if (iter == in_progress_.end()) {
    VLOG(GLOG_VINFO) << __FUNCTION__ << " response:" << identify << " not found corresponding request";
    return;
  }
  auto request = std::move(iter->second);
  in_progress_.erase(iter);
  CHECK(request->AsyncId() == identify);
  HandleResponse(request, res);
}

void AsyncChannel::OnProtocolServiceGone(const RefCodecService& service) {
  VLOG(GLOG_VTRACE) << __FUNCTION__ << service->Channel()->ChannelInfo() << " protocol service closed";
	DCHECK(IOLoop()->IsInLoopThread());
  ClientChannel::OnProtocolServiceGone(service);

  for (auto kv : in_progress_) {
    kv.second->SetFailCode(MessageCode::kConnBroken);
    HandleResponse(kv.second, CodecMessage::kNullMessage);
  }
  in_progress_.clear();
  if (delegate_) {
    auto guard = shared_from_this();
    delegate_->OnClientChannelClosed(guard);
  }
}


}}//lt::net

