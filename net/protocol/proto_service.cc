
#include "proto_service.h"
#include "proto_message.h"
#include "glog/logging.h"
#include "net/tcp_channel.h"

namespace net {
ProtoService::ProtoService()
	: type_(ProtocolServiceType::kServer) {
}

ProtoService::~ProtoService() {};

void ProtoService::SetServiceType(ProtocolServiceType t) {
  type_ = t;
}

void ProtoService::SetMessageHandler(ProtoMessageHandler handler) {
  message_handler_ = handler;
}

void ProtoService::SetDelegate(ProtoServiceDelegate* d) {
	delegate_ = d;
}

void ProtoService::BindChannel(RefTcpChannel& channel) {
	channel_ = channel;
	channel_->SetChannelConsumer(this);
}

bool ProtoService::IsConnected() const {
	return channel_ ?	channel_->IsConnected() : false;
}

void ProtoService::CloseService() {
	BeforeCloseService();
	CHECK(channel_->InIOLoop());
	if (IsConnected()) {
		channel_->ShutdownChannel();
	}
}

void ProtoService::OnChannelClosed(const RefTcpChannel& channel) {
	CHECK(channel.get() == channel_.get());
	RefProtoService guard = shared_from_this();

	AfterChannelClosed();
	VLOG(GLOG_VTRACE) << __FUNCTION__ << " " << channel_->ChannelName() << " closed";
	if (delegate_) {
		delegate_->OnProtocolServiceGone(guard);
	}
}

}// end namespace
