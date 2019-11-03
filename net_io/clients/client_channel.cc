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

  // if has client side heart beat, start it
  uint32_t heart_beat_interval = delegate_->HeartBeatInterval();
  if (heart_beat_interval > 0) {
    //TODO do client side
    //protocol_service_->StartHeartBeat(heart_beat_interval);
  }
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
  protocol_service_->CloseService();
}

void ClientChannel::ResetDelegate() {
  delegate_ = NULL;
}

//override
void ClientChannel::OnProtocolServiceReady(const RefProtoService& service) {
  state_ = kReady;
  if (delegate_) {
    delegate_->OnClientChannelInited(this);
  }
};

}}
