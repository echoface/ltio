//
// Created by gh on 18-12-15.
//

#include "client_channel.h"
#include "async_channel.h"
#include "queued_channel.h"
#include <base/utils/string/str_utils.h>

namespace lt {
namespace net {

RefClientChannel CreateClientChannel(ClientChannel::Delegate* delegate,
                                     const RefCodecService& service) {
	if (service->KeepSequence()) {
		auto client_channel = QueuedChannel::Create(delegate, service);
		return std::static_pointer_cast<ClientChannel>(client_channel);
	}
	auto client_channel = AsyncChannel::Create(delegate, service);
	return std::static_pointer_cast<ClientChannel>(client_channel);
}


ClientChannel::ClientChannel(Delegate* d, const RefCodecService& service)
		: delegate_(d),
		  codec_(service) {
  codec_->SetDelegate(this);
}

ClientChannel::~ClientChannel() {
  codec_->SetDelegate(NULL);
}

void ClientChannel::StartClientChannel() {
  codec_->StartProtocolService();
}

void ClientChannel::CloseClientChannel() {
  VLOG(GLOG_VTRACE) << __FUNCTION__ << " going to close:" << ConnectionInfo();

  state_ = kClosing;

	base::EventPump* pump = EventPump();
  CHECK(pump->IsInLoopThread());

  BeforeCloseChannel();

  delegate_ = NULL;
  if (heartbeat_timer_) {
    pump->RemoveTimeoutEvent(heartbeat_timer_);
    delete heartbeat_timer_;
    heartbeat_timer_ = NULL;
  }
  codec_->CloseService();
}

void ClientChannel::ResetDelegate() {
  delegate_ = NULL;
}

std::string ClientChannel::ConnectionInfo() const {
  const url::RemoteInfo* remote_info = GetRemoteInfo();
  if (remote_info) {
    return remote_info->HostIpPort();
  }
  return std::string("unknown connection");
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
  heartbeat_message_ = codec_->NewHeartbeat();
  return SendRequest(heartbeat_message_);
}

bool ClientChannel::HandleResponse(const RefCodecMessage& req,
                                   const RefCodecMessage& res) {
  if (heartbeat_message_.get() == req.get()) {
    heartbeat_message_.reset();
    LOG_IF(INFO, !res->IsHeartbeat()) << "heartbeat message go a none heartbeat response";
    return true;
  }

  if (delegate_) {
    delegate_->OnRequestGetResponse(req, res);
    return true;
  }

  return false;
}

//override
void ClientChannel::OnProtocolServiceGone(const RefCodecService& service) {
  base::MessageLoop* loop = IOLoop();
  if (heartbeat_timer_) {
    loop->Pump()->RemoveTimeoutEvent(heartbeat_timer_);
    delete heartbeat_timer_;
    heartbeat_timer_ = NULL;
  }
}

//override
void ClientChannel::OnProtocolServiceReady(const RefCodecService& service) {
  state_ = kReady;
  uint32_t heartbeat_ms = 0;
  VLOG(GLOG_VTRACE) << __FUNCTION__ << ConnectionInfo();
  if (delegate_) {
    delegate_->OnClientChannelInited(this);
    heartbeat_ms = delegate_->GetClientConfig().heartbeat_ms;
  }

  if (codec_->KeepHeartBeat() && heartbeat_ms > 50) {
    heartbeat_timer_ = new base::TimeoutEvent(heartbeat_ms, true);

    auto heartbeat_fun = std::bind(&ClientChannel::OnHearbeatTimerInvoke, this);
    heartbeat_timer_->InstallTimerHandler(NewClosure(heartbeat_fun));
    IOLoop()->Pump()->AddTimeoutEvent(heartbeat_timer_);
  }
};

}}
