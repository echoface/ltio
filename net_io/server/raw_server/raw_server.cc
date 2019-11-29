#include "raw_server.h"
#include "io_service.h"
#include "glog/logging.h"
#include "address.h"
#include "tcp_channel.h"
#include "url_utils.h"
#include "protocol/proto_service.h"
#include "message_loop/linux_signal.h"
#include "protocol/proto_service_factory.h"

namespace lt {
namespace net {

void RawServerContext::SendResponse(const RefProtocolMessage& response) {
  if (did_reply_) return;
  return do_response(response);
}

void RawServerContext::do_response(const RefProtocolMessage& response) {
  did_reply_ = true;
  auto service = request_->GetIOCtx().protocol_service.lock();
  if (!service) {
    LOG(ERROR) << __FUNCTION__ << " this connections has broken";
    return;
  }
  base::MessageLoop* io = service->IOLoop();

  if (io->IsInLoopThread()) {
    bool success = service->SendResponseMessage(request_.get(), response.get());
    if (!success) {
      service->CloseService();
    }
    return;
  }

  auto functor = [=]() {
    bool success = service->SendResponseMessage(request_.get(), response.get());
    if (!success) {
      service->CloseService();
    }
  };
  io->PostTask(NewClosure(std::move(functor)));
}

RawServer::RawServer():
  dispatcher_(nullptr) {
  serving_flag_.store(false);
  connection_count_.store(0);
}

void RawServer::SetIOLoops(std::vector<base::MessageLoop*>& loops) {
  if (serving_flag_.load()) {
    LOG(ERROR) << "io loops can only set before server start";
    return;
  }
  io_loops_ = loops;
}

void RawServer::SetDispatcher(Dispatcher* dispatcher) {
  dispatcher_ = dispatcher;
}

void RawServer::ServeAddressSync(const std::string addr, RawMessageHandler handler) {

  ServeAddress(addr, handler);

  std::unique_lock<std::mutex> lck(mtx_);
  while (cv_.wait_for(lck, std::chrono::milliseconds(500)) == std::cv_status::timeout) {
    LOG(INFO) << "starting... ... ...";
  }
}

void RawServer::ServeAddress(const std::string address, RawMessageHandler handler) {

  bool served = serving_flag_.exchange(true);
  LOG_IF(ERROR, served) << " Server Can't Serve Twice";
  CHECK(!served);

  CHECK(handler);
  LOG_IF(ERROR, io_loops_.empty()) << "No loop handle socket io";
  CHECK(io_loops_.size() > 0);

  url::SchemeIpPort sch_ip_port;
  if (!url::ParseURI(address, sch_ip_port)) {
    LOG(ERROR) << "address format error,eg [raw://xx.xx.xx.xx:port]";
    CHECK(false);
  }
  if (!ProtoServiceFactory::Instance().HasProtoServiceCreator(sch_ip_port.protocol)) {
    LOG(ERROR) << "No ProtoServiceCreator Find for protocol scheme:" << sch_ip_port.protocol;
    CHECK(false);
  }

  message_handler_ = handler;

  net::SocketAddr addr(sch_ip_port.host_ip, sch_ip_port.port);

  {
#if defined SO_REUSEPORT && defined NET_ENABLE_REUSER_PORT
    for (base::MessageLoop* loop : io_loops_) {
      IOServicePtr service(new IOService(addr, sch_ip_port.protocol, loop, this));
      ioservices_.push_back(std::move(service));
    }
#else
    base::MessageLoop* loop = io_loops_[ioservices_.size() % io_loops_.size()];
    IOServicePtr service(new IOService(addr, sch_ip_port.protocol, loop, this));
    ioservices_.push_back(std::move(service));
#endif

    for (IOServicePtr& service : ioservices_) {
      service->StartIOService();
    }
  }
}

void RawServer::OnRequestMessage(const RefProtocolMessage& request) {
  VLOG(GLOG_VTRACE) << "IOService::HandleRequest handle a request";

  if (!dispatcher_) {
    HandleRawRequest(request);
    return;
  }

  base::StlClosure functor = std::bind(&RawServer::HandleRawRequest, this, request);
  if (false == dispatcher_->Dispatch(functor)) {
    LOG(ERROR) << __FUNCTION__ << " dispatcher_->Dispatch failed";
    auto proto_service = request->GetIOCtx().protocol_service.lock();
    if (proto_service) {
      proto_service->CloseService();
    }
  }
}

//run on loop handle request
void RawServer::HandleRawRequest(const RefProtocolMessage request) {

  auto service = request->GetIOCtx().protocol_service.lock();
  if (!service) {
    VLOG(GLOG_VERROR) << __FUNCTION__ << " Channel Has Broken After Handle Request Message";
    return;
  }

  RawServerContext context(request);
  do {
    if (dispatcher_ && !dispatcher_->SetWorkContext(request.get())) {
      LOG(ERROR) << __FUNCTION__ << " Set WorkerContext failed";
      break;
    }
    message_handler_(&context);
  } while(0);

  if (!context.IsResponded()) {
    auto response = service->NewResponse(request.get());
    context.SendResponse(response);
  }
}

bool RawServer::CanCreateNewChannel() {
  return true;
}

base::MessageLoop* RawServer::GetNextIOWorkLoop() {
#if defined SO_REUSEPORT && defined NET_ENABLE_REUSER_PORT
  return base::MessageLoop::Current();
#else
  static std::atomic_uint32_t index = 0;
  return io_loops_[index.fetch_add(1) % io_loops_.size()];
#endif
}

bool RawServer::IncreaseChannelCount() {
  connection_count_.fetch_add(1);
  LOG(INFO) << __FUNCTION__ << " rawserver connections +1 count:" << connection_count_;
  return true;
}

void RawServer::DecreaseChannelCount() {
  connection_count_.fetch_sub(1);
  LOG(INFO) << __FUNCTION__ << " rawserver connections -1 count:" << connection_count_;
}

bool RawServer::BeforeIOServiceStart(IOService* ioservice) {
  return true;
}

void RawServer::IOServiceStarted(const IOService* service) {
  LOG(INFO) << "Raw Server IO Service " << service->IOServiceName() << " Started .......";

  { //sync
    std::unique_lock<std::mutex> lck(mtx_);
    for (auto& service : ioservices_) {
      if (!service->IsRunning()) return;
    }
    cv_.notify_all();
  }
}

void RawServer::IOServiceStoped(const IOService* service) {
  LOG(INFO) << "Raw Server IO Service " << service->IOServiceName() << " Stoped  .......";

  {
    std::unique_lock<std::mutex> lck(mtx_);
    ioservices_.remove_if([&](IOServicePtr& s) -> bool {
      return s.get() == service;
    });
  }

  { //sync
    std::unique_lock<std::mutex> lck(mtx_);
    for (auto& service : ioservices_) {
      if (service->IsRunning()) return;
    }
    cv_.notify_all();
  }
}

void RawServer::StopServerSync() {

  StopServer();

  std::unique_lock<std::mutex> lck(mtx_);
  while (cv_.wait_for(lck, std::chrono::milliseconds(500)) == std::cv_status::timeout) {
    LOG(INFO) << "stoping... ... ...";
  }
}

void RawServer::StopServer() {
  for (auto& service : ioservices_) {
    service->StopIOService();
  }
}

}} //end net
