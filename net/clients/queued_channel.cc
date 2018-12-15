#include "async_channel.h"
#include "queued_channel.h"
#include "base/base_constants.h"
#include "protocol/proto_service.h"

namespace net {

const static RefProtocolMessage kNullResponse;

RefClientChannel CreateClientChannel(ClientChannel::Delegate* delegate, RefProtoService& service) {
  if (service->KeepSequence()) {
    auto client_channel = std::make_shared<QueuedChannel>(delegate, service);
    return std::static_pointer_cast<ClientChannel>(client_channel);
  }
  auto client_channel = std::make_shared<AsyncChannel>(delegate, service);
  return std::static_pointer_cast<ClientChannel>(client_channel);
}

QueuedChannel::QueuedChannel(Delegate* d, RefProtoService& service)
  : ClientChannel(d, service) {
}

QueuedChannel::~QueuedChannel() {
}

void QueuedChannel::StartClient() {

  ProtoMessageHandler res_handler =
      std::bind(&QueuedChannel::OnResponseMessage,this, std::placeholders::_1);

  protocol_service_->SetMessageHandler(std::move(res_handler));

  protocol_service_->Channel()->Start();
}

void QueuedChannel::SendRequest(RefProtocolMessage request)  {
  CHECK(IOLoop()->IsInLoopThread());

  request->SetIOContext(protocol_service_);
  waiting_list_.push_back(std::move(request));

  TrySendNext();
}

bool QueuedChannel::TrySendNext() {
  if (in_progress_request_) {
    return true;
  }

  bool success = false;
  while(!success && waiting_list_.size()) {

    RefProtocolMessage& next = waiting_list_.front();
    success = protocol_service_->SendRequestMessage(next);
    waiting_list_.pop_front();
    if (success) {
      in_progress_request_ = next;
    } else {
      next->SetFailInfo(FailInfo::kChannelBroken);
      LOG(INFO) << __FUNCTION__ << " send failed call OnrequestGetResponse";
      delegate_->OnRequestGetResponse(next, kNullResponse);
    }
  }

  if (success) {
    CHECK(in_progress_request_);
    WeakProtocolMessage weak(in_progress_request_);   // weak ptr must init outside, Take Care of weakptr
    auto functor = std::bind(&QueuedChannel::OnRequestTimeout, shared_from_this(), weak);
    IOLoop()->PostDelayTask(NewClosure(functor), request_timeout_);
  }
  return success;
}

void QueuedChannel::OnRequestTimeout(WeakProtocolMessage weak) {

  RefProtocolMessage request = weak.lock();
  if (!request) return;

  if (request.get() != in_progress_request_.get()) {
    return;
  }

  VLOG(GLOG_VINFO) << __FUNCTION__ << protocol_service_->Channel()->ChannelInfo() << " timeout reached";
  request->SetFailInfo(FailInfo::kTimeOut);
  delegate_->OnRequestGetResponse(request, kNullResponse);
  in_progress_request_.reset();

  if (protocol_service_->IsConnected()) {
  	protocol_service_->CloseService();
  } else {
  	OnProtocolServiceGone(protocol_service_);
  }
}

void QueuedChannel::OnResponseMessage(const RefProtocolMessage& res) {
  //CHECK(IOLoop()->IsInLoopThread());

  if (!in_progress_request_) {
    LOG(ERROR) << "Got Response Without Request or Request Has Canceled";
    return ;
  }

  LOG(INFO) << __FUNCTION__ << " call OnrequestGetResponse";
  delegate_->OnRequestGetResponse(in_progress_request_, res);
  in_progress_request_.reset();

  TrySendNext();
}

void QueuedChannel::OnProtocolServiceGone(const RefProtoService& service) {
  VLOG(GLOG_VTRACE) << __FUNCTION__ << service->Channel()->ChannelInfo() << " protocol service closed";

  if (in_progress_request_) {
    in_progress_request_->SetFailInfo(FailInfo::kChannelBroken);
    delegate_->OnRequestGetResponse(in_progress_request_, kNullResponse);
    in_progress_request_.reset();
  }

  while(waiting_list_.size()) {
    RefProtocolMessage& request = waiting_list_.front();
    request->SetFailInfo(FailInfo::kChannelBroken);
    delegate_->OnRequestGetResponse(request, kNullResponse);
    waiting_list_.pop_front();
  }

  auto guard = shared_from_this();
  delegate_->OnClientChannelClosed(guard);
}

}//net
