#include "codec_service.h"
#include "codec_message.h"

#include "glog/logging.h"
#include <net_io/tcp_channel.h>
#include <base/message_loop/message_loop.h>

namespace lt {
namespace net {

CodecService::CodecService(base::MessageLoop* loop)
  : binded_loop_(loop){
}

void CodecService::SetDelegate(CodecService::Delegate* d) {
	delegate_ = d;
}

bool CodecService::IsConnected() const {
	return channel_ ? channel_->IsConnected() : false;
}

bool CodecService::BindToSocket(int fd,
                                const IPEndPoint& local,
                                const IPEndPoint& peer) {

  channel_ = TcpChannel::Create(fd, local, peer, binded_loop_->Pump());
	channel_->SetReciever(this);
  return true;
}

void CodecService::StartProtocolService() {
  CHECK(binded_loop_->IsInLoopThread());

  channel_->StartChannel();
}

void CodecService::CloseService() {
	CHECK(binded_loop_->IsInLoopThread());

	BeforeCloseService();
	channel_->ShutdownChannel(false);
}

void CodecService::SetIsServerSide(bool server_side) {
  server_side_ = server_side;
}

void CodecService::OnChannelClosed(const SocketChannel* channel) {
	CHECK(channel == channel_.get());

	VLOG(GLOG_VTRACE) << __FUNCTION__ << channel_->ChannelInfo() << " closed";

	RefCodecService guard = shared_from_this();
	AfterChannelClosed();
	if (delegate_) {
		delegate_->OnProtocolServiceGone(guard);
	}
}

void CodecService::OnChannelReady(const SocketChannel*) {
  RefCodecService guard = shared_from_this();
  if (delegate_) {
    delegate_->OnProtocolServiceReady(guard);
  }
}

}}// end namespace
