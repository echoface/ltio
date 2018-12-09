

#include "io_service.h"
#include "glog/logging.h"
#include "tcp_channel.h"
#include "protocol/proto_service.h"
#include "protocol/proto_message.h"
#include "protocol/proto_service_factory.h"

#include <atomic>
#include <base/base_constants.h>

namespace net {

IOService::IOService(const InetAddress addr,
                     const std::string protocol,
                     base::MessageLoop* workloop,
                     IOServiceDelegate* delegate)
  : protocol_(protocol),
    acceptor_loop_(workloop),
    delegate_(delegate),
    is_stopping_(false) {

  CHECK(delegate_);

  service_name_ = addr.IpPortAsString();
  acceptor_.reset(new ServiceAcceptor(acceptor_loop_->Pump(), addr));
  acceptor_->SetNewConnectionCallback(std::bind(&IOService::OnNewConnection,
                                                this,
                                                std::placeholders::_1,
                                                std::placeholders::_2));
}

IOService::~IOService() {
  LOG(INFO) << "IOServce protocol " << protocol_ << " Gone";
}

void IOService::StartIOService() {

  if (!acceptor_loop_->IsInLoopThread()) {
    auto functor = std::bind(&IOService::StartIOService, this);
    acceptor_loop_->PostTask(NewClosure(std::move(functor)));
    return;
  }

  CHECK(acceptor_->StartListen());

  delegate_->IOServiceStarted(this);
}

/* step1: close the acceptor */
void IOService::StopIOService() {
  if (!acceptor_loop_->IsInLoopThread()) {
    auto functor = std::bind(&IOService::StopIOService, this);
    acceptor_loop_->PostTask(NewClosure(std::move(functor)));
    return;
  }

  CHECK(acceptor_loop_->IsInLoopThread());

  //sync
  acceptor_->StopListen();
  is_stopping_ = true;

  //async
  for (auto& proto_service : protocol_services) {
      base::MessageLoop* loop = proto_service->IOLoop();
      loop->PostTask(NewClosure(std::bind(&ProtoService::CloseService, proto_service)));
  }

  if (protocol_services.size() == 0) {
    delegate_->IOServiceStoped(this);
  }
}

void IOService::OnNewConnection(int local_socket, const InetAddress& peer_addr) {
  CHECK(acceptor_loop_->IsInLoopThread());

  //max connction reached
  if (!delegate_->CanCreateNewChannel()) {
    socketutils::CloseSocket(local_socket);
    LOG(INFO) << "Max Channels Limit, Current Has:[" << protocol_services.size() << "] Channels";
    return;
  }

  base::MessageLoop* io_loop = delegate_->GetNextIOWorkLoop();
  if (!io_loop) {
    socketutils::CloseSocket(local_socket);
    return;
  }

  RefProtoService proto_service = ProtoServiceFactory::Create(protocol_);
  if (!proto_service || !message_handler_) {
    LOG(ERROR) << "no proto parser or no message handler, close this connection.";
    socketutils::CloseSocket(local_socket);
    return;
  }
  proto_service->SetDelegate(this);
  proto_service->SetMessageHandler(message_handler_);
  proto_service->SetServiceType(ProtocolServiceType::kServer);

  net::InetAddress local_addr(socketutils::GetLocalAddrIn(local_socket));

  auto new_channel = TcpChannel::Create(local_socket, local_addr, peer_addr, io_loop);

  const std::string channel_name = peer_addr.IpPortAsString();
  new_channel->SetChannelName(channel_name);

  proto_service->BindChannel(new_channel);

  new_channel->Start();

  VLOG(GLOG_VTRACE) << " New Connection from:" << channel_name;
  StoreProtocolService(proto_service);
}

void IOService::OnProtocolServiceGone(const net::RefProtoService &service) {
  if (!acceptor_loop_->IsInLoopThread()) {
    auto functor = std::bind(&IOService::OnProtocolServiceGone, this, service);
    acceptor_loop_->PostTask(NewClosure(std::move(functor)));
    return;
  }
  LOG(INFO) << __FUNCTION__ << " a channel broken, remove this protocol service";
  RemoveProtocolService(service);
}

void IOService::StoreProtocolService(const RefProtoService service) {
  CHECK(acceptor_loop_->IsInLoopThread());
  protocol_services.insert(service);
  LOG(INFO) << __FUNCTION__ << " keep a new protocol service, count:" << protocol_services.size();
  delegate_->IncreaseChannelCount();
}

void IOService::RemoveProtocolService(const RefProtoService service) {
  CHECK(acceptor_loop_->IsInLoopThread());
  protocol_services.erase(service);

  LOG(INFO) << __FUNCTION__ << " remove a new protocol service, count:" << protocol_services.size();

  delegate_->DecreaseChannelCount();
  if (is_stopping_ && protocol_services.size() == 0) {
    delegate_->IOServiceStoped(this);
  }
}

void IOService::SetProtoMessageHandler(ProtoMessageHandler handler) {
  message_handler_ = handler;
}

}// endnamespace net
