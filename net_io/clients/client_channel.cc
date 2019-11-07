//
// Created by gh on 18-12-15.
//

#include "client_channel.h"
#include "async_channel.h"
#include "queued_channel.h"

namespace lt {
namespace net {

RefClientChannel CreateClientChannel(ClientChannel::Delegate* delegate,
                                     const RefProtoService& service) {
	if (service->KeepSequence()) {
		auto client_channel = QueuedChannel::Create(delegate, service);
		return std::static_pointer_cast<ClientChannel>(client_channel);
	}
	auto client_channel = AsyncChannel::Create(delegate, service);
	return std::static_pointer_cast<ClientChannel>(client_channel);
}


ClientChannel::ClientChannel(Delegate* d, const RefProtoService& service)
		: delegate_(d),
		  protocol_service_(service) {
}

ClientChannel::~ClientChannel() {
  protocol_service_->SetDelegate(NULL);
}

void ClientChannel::StartClient() {
  protocol_service_->SetDelegate(this);
  protocol_service_->Initialize();
}

void ClientChannel::Close() {
	base::MessageLoop* io = IOLoop();
  CHECK(io->IsInLoopThread());
  state_ = kClosing;
  VLOG(GLOG_VTRACE) << " close client channel";
  //notify impl to close all in progress request
  BeforeCloseChannel();

  delegate_ = NULL;
  if (heartbeat_timer_) {
    IOLoop()->Pump()->RemoveTimeoutEvent(heartbeat_timer_);
    delete heartbeat_timer_;
    heartbeat_timer_ = NULL;
  }
  protocol_service_->CloseService();
}

void ClientChannel::ResetDelegate() {
  delegate_ = NULL;
}

//override
const url::RemoteInfo* ClientChannel::GetRemoteInfo() const {
  if (delegate_) {
    return &(delegate_->GetRemoteInfo());
  }
  return NULL;
}

void ClientChannel::OnHearbeatTimerInvoke() {
  if (heartbeat_message_) {
    LOG(ERROR) << __FUNCTION__ << " heartbeat snowball, timeout_ms > heartbeat_ms?"; 
    return;
  }
  heartbeat_message_ = protocol_service_->NewHeartbeat();
  LOG(INFO) << __FUNCTION__ << " send heartbeat message";
  return SendRequest(heartbeat_message_);
}

bool ClientChannel::HandleResponse(const RefProtocolMessage& req,
                                   const RefProtocolMessage& res) {
  if (heartbeat_message_.get() == req.get()) {
    LOG(ERROR) << __FUNCTION__ << " heartbeat response, alive";
    heartbeat_message_.reset();
    return true;
  }
  if (delegate_) {
    delegate_->OnRequestGetResponse(req, res);
    return true;
  }
  return false;
}

//override
void ClientChannel::OnProtocolServiceGone(const RefProtoService& service) {
  if (heartbeat_timer_) {
    IOLoop()->Pump()->RemoveTimeoutEvent(heartbeat_timer_);
    delete heartbeat_timer_;
    heartbeat_timer_ = NULL;
  }

}

//override
void ClientChannel::OnProtocolServiceReady(const RefProtoService& service) {
  state_ = kReady;
  uint32_t heartbeat_ms = 0;
  if (delegate_) {
    delegate_->OnClientChannelInited(this);
    heartbeat_ms = delegate_->GetClientConfig().heartbeat_ms;
  }
  if (protocol_service_->KeepHeartBeat() && heartbeat_ms > 50) {
    heartbeat_timer_ = new base::TimeoutEvent(heartbeat_ms, true);

    auto heartbeat_fun = std::bind(&ClientChannel::OnHearbeatTimerInvoke, this);
    heartbeat_timer_->InstallTimerHandler(NewClosure(heartbeat_fun));
    IOLoop()->Pump()->AddTimeoutEvent(heartbeat_timer_);
  }
};

}}
