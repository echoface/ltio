

#include "io_service.h"
#include "glog/logging.h"
#include "tcp_channel.h"
#include "protocol/proto_service.h"
#include "protocol/proto_message.h"
#include "protocol/proto_service_factory.h"

#include <atomic>
#include <base/base_constants.h>

namespace net {

IOService::IOService(const SocketAddress addr,
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
  for (auto& proto_service : protocol_services) {
      base::MessageLoop* loop = proto_service->IOLoop();
      loop->PostTask(NewClosure(std::bind(&ProtoService::CloseService, proto_service)));
  }

  if (protocol_services.empty()) {
    delegate_->IOServiceStoped(this);
  }
}

void IOService::OnNewConnection(int fd, const SocketAddress& peer_addr) {

  CHECK(accept_loop_->IsInLoopThread());

  //check connection limit and others
  if (!delegate_->CanCreateNewChannel()) {
    socketutils::CloseSocket(fd);
    LOG(INFO) << "Max Channels Limit, Current Has:[" << protocol_services.size() << "] Channels";
    return;
  }

  base::MessageLoop* io_loop = delegate_->GetNextIOWorkLoop();
  if (!io_loop) {
    socketutils::CloseSocket(fd);
    return;
  }

  RefProtoService proto_service = ProtoServiceFactory::Create(protocol_, true);
  if (!proto_service || !message_handler_) {
    LOG(ERROR) << "no proto parser or no message handler, close this connection.";
    socketutils::CloseSocket(fd);
    return;
  }
  SocketAddress local_addr(socketutils::GetLocalAddrIn(fd));

  proto_service->SetDelegate(this);
  proto_service->SetMessageHandler(message_handler_);

  proto_service->BindChannel(fd, local_addr, peer_addr, io_loop);

  StoreProtocolService(proto_service);

  VLOG(GLOG_VTRACE) << " New Connection from:" << peer_addr.IpPort() << " establisted";
}

void IOService::OnProtocolServiceGone(const net::RefProtoService &service) {
  // use another task remove a service is a more safe way delete channel& protocol things
  // avoid somewhere->B(do close a channel) ->  ~A  -> use A again in somewhere
  auto functor = std::bind(&IOService::RemoveProtocolService, this, service);
  accept_loop_->PostTask(NewClosure(std::move(functor)));
}

void IOService::StoreProtocolService(const RefProtoService service) {
  CHECK(accept_loop_->IsInLoopThread());

  protocol_services.insert(service);
  delegate_->IncreaseChannelCount();
}

void IOService::RemoveProtocolService(const RefProtoService service) {
  CHECK(accept_loop_->IsInLoopThread());
  protocol_services.erase(service);

  delegate_->DecreaseChannelCount();
  if (is_stopping_ && protocol_services.size() == 0) {
    delegate_->IOServiceStoped(this);
  }
}

void IOService::SetProtoMessageHandler(ProtoMessageHandler handler) {
  message_handler_ = handler;
}

}// endnamespace net
