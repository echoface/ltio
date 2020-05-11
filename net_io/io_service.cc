#include <atomic>
#include "base/ip_endpoint.h"
#include "glog/logging.h"
#include <base/base_constants.h>
#include "base/closure/closure_task.h"

#include "io_service.h"
#include "tcp_channel.h"
#include "codec/codec_service.h"
#include "codec/codec_message.h"
#include "codec/codec_factory.h"

namespace lt {
namespace net {

IOService::IOService(const IPEndPoint& addr,
                     const std::string protocol,
                     base::MessageLoop* ioloop,
                     IOServiceDelegate* delegate)
  : protocol_(protocol),
    accept_loop_(ioloop),
    delegate_(delegate),
    is_stopping_(false) {

  CHECK(delegate_);

  service_name_ = addr.ToString();
  acceptor_.reset(new SocketAcceptor(accept_loop_->Pump(), addr));
  acceptor_->SetNewConnectionCallback(std::bind(&IOService::OnNewConnection,
                                                this,
                                                std::placeholders::_1,
                                                std::placeholders::_2));
}

IOService::~IOService() {
  LOG(INFO) << __FUNCTION__ << "IOServce gone, this:" << this;
}

void IOService::StartIOService() {

  if (!accept_loop_->IsInLoopThread()) {
    auto functor = std::bind(&IOService::StartIOService, this);
    accept_loop_->PostTask(NewClosure(std::move(functor)));
    return;
  }

  CHECK(acceptor_->StartListen());

  if (delegate_) {
    delegate_->IOServiceStarted(this);
  }
}

/* step1: close the acceptor */
void IOService::StopIOService() {
  if (!accept_loop_->IsInLoopThread()) {
    auto functor = std::bind(&IOService::StopIOService, this);
    accept_loop_->PostTask(NewClosure(std::move(functor)));
    return;
  }

  CHECK(accept_loop_->IsInLoopThread());

  //sync
  acceptor_->StopListen();
  is_stopping_ = true;

  //async
  for (auto& codec_service : codecs_) {
    base::MessageLoop* loop = codec_service->IOLoop();
    loop->PostTask(NewClosure(std::bind(&CodecService::CloseService, codec_service)));
  }

  if (codecs_.empty() && delegate_) {
    delegate_->IOServiceStoped(this);
  }
}

void IOService::OnNewConnection(int fd, const IPEndPoint& peer_addr) {
  CHECK(accept_loop_->IsInLoopThread());

  VLOG(GLOG_VTRACE) << __FUNCTION__ << " connect apply from:" << peer_addr.ToString();

  //check connection limit and others
  if (!delegate_ || !delegate_->CanCreateNewChannel()) {
    LOG(INFO) << __FUNCTION__ << " Stop accept new connection"
      << ", current has:[" << codecs_.size() << "]";
    return socketutils::CloseSocket(fd);
  }

  base::MessageLoop* io_loop = delegate_->GetNextIOWorkLoop();
  if (!io_loop) {
    LOG(INFO) << __FUNCTION__ << " No IOLoop handle this connect request";
    return socketutils::CloseSocket(fd);
  }

  RefCodecService codec_service =
    CodecFactory::NewServerService(protocol_, io_loop);

  if (!codec_service) {
    LOG(ERROR) << __FUNCTION__ << " scheme:" << protocol_ << " NOT-FOUND";
    return socketutils::CloseSocket(fd);
  }

  IPEndPoint local_addr;
  CHECK(socketutils::GetLocalEndpoint(fd, &local_addr));

  codec_service->SetDelegate(this);
  codec_service->BindToSocket(fd, local_addr, peer_addr);

  if (io_loop->IsInLoopThread()) {
    codec_service->StartProtocolService();
  } else {
    io_loop->PostTask(NewClosure(
        std::bind(&CodecService::StartProtocolService, codec_service)));
  }
  StoreProtocolService(codec_service);

  VLOG(GLOG_VTRACE) << __FUNCTION__
    << " Connection from:" << peer_addr.ToString() << " establisted";
}

void IOService::OnCodecMessage(const RefCodecMessage& message) {
  if (delegate_) {
    return delegate_->OnRequestMessage(message);
  }
  LOG(INFO) << __func__ <<  " nobody handle this request";
}

void IOService::OnProtocolServiceGone(const net::RefCodecService &service) {
  // use another task remove a service is a more safe way delete channel& protocol things
  // avoid somewhere->B(do close a channel) ->  ~A  -> use A again in somewhere
  auto functor = std::bind(&IOService::RemoveProtocolService, this, service);
  accept_loop_->PostTask(NewClosure(std::move(functor)));
}

void IOService::StoreProtocolService(const RefCodecService service) {
  CHECK(accept_loop_->IsInLoopThread());

  codecs_.insert(service);
  delegate_->IncreaseChannelCount();
}

void IOService::RemoveProtocolService(const RefCodecService service) {
  CHECK(accept_loop_->IsInLoopThread());

  codecs_.erase(service);

  delegate_->DecreaseChannelCount();
  if (delegate_ && is_stopping_ && codecs_.empty()) {
    delegate_->IOServiceStoped(this);
  }
}

}}// endnamespace net
