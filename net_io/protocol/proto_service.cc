
#include "proto_service.h"
#include "proto_message.h"
#include "glog/logging.h"

#include "tcp_channel.h"

namespace lt {
namespace net {

ProtoService::ProtoService(){}
ProtoService::~ProtoService() {};

void ProtoService::SetDelegate(ProtoServiceDelegate* d) {
	delegate_ = d;
}

bool ProtoService::IsConnected() const {
	return channel_ ? channel_->IsConnected() : false;
}

bool ProtoService::BindToSocket(int fd,
                                const SocketAddress& local,
                                const SocketAddress& peer,
                                base::MessageLoop* loop) {

  channel_ = TcpChannel::Create(fd, local, peer, loop);
	channel_->SetReciever(this);

  channel_->Start();

  return true;
}

void ProtoService::CloseService() {
	CHECK(channel_->InIOLoop());

	BeforeCloseService();
	channel_->ShutdownChannel();
}

void ProtoService::OnChannelClosed(const SocketChannel* channel) {
	CHECK(channel == channel_.get());

	VLOG(GLOG_VTRACE) << __FUNCTION__ << channel_->ChannelInfo() << " closed";

	RefProtoService guard = shared_from_this();
	AfterChannelClosed();
	if (delegate_) {
		delegate_->OnProtocolServiceGone(guard);
	}
}

}}// end namespace
