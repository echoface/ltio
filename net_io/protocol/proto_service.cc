#include "proto_service.h"
#include "base/message_loop/message_loop.h"
#include "proto_message.h"

#include "glog/logging.h"
#include <net_io/tcp_channel.h>

namespace lt {
namespace net {

ProtoService::ProtoService(base::MessageLoop* loop)
  : binded_loop_(loop){
}

void ProtoService::SetDelegate(ProtoServiceDelegate* d) {
	delegate_ = d;
}

bool ProtoService::IsConnected() const {
	return channel_ ? channel_->IsConnected() : false;
}

bool ProtoService::BindToSocket(int fd,
                                const SocketAddr& local,
                                const SocketAddr& peer) {
  channel_ = TcpChannel::Create(fd, local, peer, binded_loop_->Pump());

	channel_->SetReciever(this);
  return true;
}

void ProtoService::Initialize() {
  channel_->StartChannel();
  //on client side, do the setup-things(auth,login,db) when channel ready callback
}

void ProtoService::CloseService() {
	CHECK(binded_loop_->IsInLoopThread());

	BeforeCloseService();
	channel_->ShutdownChannel(false);
}

void ProtoService::SetIsServerSide(bool server_side) {
  server_side_ = server_side;
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

void ProtoService::OnChannelReady(const SocketChannel*) {
  RefProtoService guard = shared_from_this();
  if (delegate_) {
    delegate_->OnProtocolServiceReady(guard);
  }
}

}}// end namespace
