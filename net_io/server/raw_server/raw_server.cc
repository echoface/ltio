#include "glog/logging.h"
#include "net_io/tcp_channel.h"
#include "net_io/url_utils.h"
#include "net_io/io_service.h"
#include "net_io/codec/codec_service.h"
#include "base/message_loop/linux_signal.h"
#include "net_io/codec/codec_factory.h"

#include <algorithm>
#include "raw_server.h"

namespace lt {
namespace net {

void RawServerContext::SendResponse(const RefCodecMessage& response) {
  if (did_reply_) return;
  return do_response(response);
}

void RawServerContext::do_response(const RefCodecMessage& response) {
  did_reply_ = true;
  auto service = request_->GetIOCtx().codec.lock();
  if (!service) {
    LOG(ERROR) << __FUNCTION__ << " this connections has broken";
    return;
  }
  base::MessageLoop* io = service->IOLoop();

  if (io->IsInLoopThread()) {
    bool success = service->EncodeResponseToChannel(request_.get(), response.get());
    if (!success) {
      service->CloseService();
    }
    return;
  }

  auto functor = [=]() {
    bool success = service->EncodeResponseToChannel(request_.get(), response.get());
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

void RawServer::ServeAddress(const std::string& address, RawMessageHandler handler) {

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
  if (!CodecFactory::HasCreator(sch_ip_port.protocol)) {
    LOG(ERROR) << "No CodecServiceCreator Find for protocol scheme:" << sch_ip_port.protocol;
    CHECK(false);
  }

  message_handler_ = handler;

  net::IPEndPoint addr(sch_ip_port.host_ip, sch_ip_port.port);
  {
#if defined SO_REUSEPORT && defined NET_ENABLE_REUSER_PORT
    for (base::MessageLoop* loop : io_loops_) {
      RefIOService service(new IOService(addr, sch_ip_port.protocol, loop, this));
      ioservices_.push_back(std::move(service));
    }
#else
    base::MessageLoop* loop = io_loops_[ioservices_.size() % io_loops_.size()];
    IOServicePtr service(new IOService(addr, sch_ip_port.protocol, loop, this));
    ioservices_.push_back(std::move(service));
#endif

    for (RefIOService& service : ioservices_) {
      service->StartIOService();
    }
  }
}

void RawServer::OnRequestMessage(const RefCodecMessage& request) {
  VLOG(GLOG_VTRACE) << "IOService::HandleRequest handle a request";

  if (!dispatcher_) {
    HandleRawRequest(request);
    return;
  }

  base::LtClosure functor = std::bind(&RawServer::HandleRawRequest, this, request);

  if (false == dispatcher_->Dispatch(functor)) {
    LOG(ERROR) << __FUNCTION__ << " dispatcher_->Dispatch failed";
    auto codec_service = request->GetIOCtx().codec.lock();
    if (codec_service) {
      codec_service->CloseService();
    }
  }
}

//run on loop handle request
void RawServer::HandleRawRequest(const RefCodecMessage request) {

  auto service = request->GetIOCtx().codec.lock();
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

void RawServer::IncreaseChannelCount() {
  connection_count_.fetch_add(1);
  VLOG(GLOG_VTRACE) << __func__ << " connections +1 now:" << connection_count_;
}

void RawServer::DecreaseChannelCount() {
  uint64_t count = connection_count_.fetch_sub(1);
  VLOG(GLOG_VTRACE) << __func__ << " connections -1 now:" << count + 1;
}

void RawServer::IOServiceStarted(const IOService* service) {
  LOG(INFO) << "RawServer IOService:[" << service->IOServiceName() << "] Started";
}

void RawServer::SetCloseCallback(const base::ClosureCallback& callback) {
  closed_callback_ = callback;
}

void RawServer::IOServiceStoped(const IOService* service) {
  LOG(INFO) << "RawServer IOService:[" << service->IOServiceName() << "] Stoped";
  {
    std::unique_lock<std::mutex> lck(mtx_);
    ioservices_.remove_if([&](RefIOService& s) -> bool {
      return s.get() == service;
    });

    LOG_IF(INFO, ioservices_.empty()) << __func__ << " RawServer stoped Done";
    if (ioservices_.empty() && closed_callback_) {
      closed_callback_();
    }
  }
}

void RawServer::StopServer() {
  std::list<RefIOService> services = ioservices_;

  std::for_each(services.begin(),
                services.end(),
                [](const RefIOService& service) {
                  base::MessageLoop* io = service->AcceptorLoop();
                  io->PostTask(FROM_HERE, &IOService::StopIOService, service);
                });;
}

}} //end net
