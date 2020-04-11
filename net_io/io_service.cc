#include <atomic>
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

IOService::IOService(const SocketAddr addr,
                     const std::string protocol,
                     base::MessageLoop* ioloop,
                     IOServiceDelegate* delegate)
  : protocol_(protocol),
    accept_loop_(ioloop),
    delegate_(delegate),
    is_stopping_(false) {

  CHECK(delegate_);

  service_name_ = addr.IpPort();
  acceptor_.reset(new SocketAcceptor(accept_loop_->Pump(), addr));
  acceptor_->SetNewConnectionCallback(std::bind(&IOService::OnNewConnection,
                                                this,
                                                std::placeholders::_1,
                                                std::placeholders::_2));
}

IOService::~IOService() {
  LOG(INFO) << "IOServce protocol " << protocol_ << " Gone";
}

void IOService::StartIOService() {

  if (!accept_loop_->IsInLoopThread()) {
    auto functor = std::bind(&IOService::StartIOService, this);
    accept_loop_->PostTask(NewClosure(std::move(functor)));
    return;
  }

  CHECK(acceptor_->StartListen());

  delegate_->IOServiceStarted(this);
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
  for (auto& codec_service : codecs) {
    base::MessageLoop* loop = codec_service->IOLoop();
    loop->PostTask(NewClosure(std::bind(&CodecService::CloseService, codec_service)));
  }

  if (codecs.empty()) {
    delegate_->IOServiceStoped(this);
  }
}

void IOService::OnNewConnection(int fd, const SocketAddr& peer_addr) {
  CHECK(accept_loop_->IsInLoopThread());

  VLOG(GLOG_VTRACE) << __FUNCTION__ << " connect apply from:" << peer_addr.IpPort();

  //check connection limit and others
  if (!delegate_->CanCreateNewChannel()) {
    LOG(INFO) << __FUNCTION__ << " Stop accept new connection"
      << ", current has:[" << codecs.size() << "]";
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

  SocketAddr local_addr(socketutils::GetLocalAddrIn(fd));

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
    << " Connection from:" << peer_addr.IpPort() << " establisted";
}

void IOService::OnCodecMessage(const RefCodecMessage& message) {
  delegate_->OnRequestMessage(message);
}

void IOService::OnProtocolServiceGone(const net::RefCodecService &service) {
  // use another task remove a service is a more safe way delete channel& protocol things
  // avoid somewhere->B(do close a channel) ->  ~A  -> use A again in somewhere
  auto functor = std::bind(&IOService::RemoveProtocolService, this, service);
  accept_loop_->PostTask(NewClosure(std::move(functor)));
}

void IOService::StoreProtocolService(const RefCodecService service) {
  CHECK(accept_loop_->IsInLoopThread());

  codecs.insert(service);
  delegate_->IncreaseChannelCount();
}

void IOService::RemoveProtocolService(const RefCodecService service) {
  CHECK(accept_loop_->IsInLoopThread());
  codecs.erase(service);

  delegate_->DecreaseChannelCount();
  if (is_stopping_ && codecs.size() == 0) {
    delegate_->IOServiceStoped(this);
  }
}

}}// endnamespace net
