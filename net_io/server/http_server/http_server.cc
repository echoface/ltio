#include "glog/logging.h"
#include "net_io/io_service.h"
#include "net_io/address.h"
#include "net_io/tcp_channel.h"
#include "net_io/url_utils.h"
#include "net_io/protocol/proto_service.h"
#include "net_io/protocol/proto_service_factory.h"
#include "base/message_loop/linux_signal.h"
#include "base/coroutine/coroutine_runner.h"

#include "http_server.h"

namespace lt {
namespace net {

HttpServer::HttpServer() {
  serving_flag_.store(false);
  connection_count_.store(0);
}
HttpServer::~HttpServer() {
  serving_flag_.store(false);
}

void HttpServer::SetDispatcher(Dispatcher* dispatcher) {
  dispatcher_ = dispatcher;
}

void HttpServer::SetIOLoops(std::vector<base::MessageLoop*>& loops) {
  if (serving_flag_) {
    LOG(ERROR) << "io loops can only set before http server start";
    return;
  }
  io_loops_ = loops;
}

void HttpServer::ServeAddressSync(const std::string addr, HttpMessageHandler handler) {
  ServeAddress(addr, handler);

  std::unique_lock<std::mutex> lck(mtx_);
  while (cv_.wait_for(lck, std::chrono::milliseconds(500)) == std::cv_status::timeout) {
    LOG(INFO) << "starting... ... ...";
  }
}

void HttpServer::ServeAddress(const std::string address, HttpMessageHandler handler) {

  bool served = serving_flag_.exchange(true);
  LOG_IF(ERROR, served) << " Server Can't Serve Twice";
  CHECK(!served);

  url::SchemeIpPort sch_ip_port;
  if (!url::ParseURI(address, sch_ip_port)) {
    LOG(ERROR) << "address format error,eg [http://xx.xx.xx.xx:port]";
    CHECK(false);
  }
  if (!ProtoServiceFactory::Instance().HasProtoServiceCreator(sch_ip_port.protocol)) {
    LOG(ERROR) << "No ProtoServiceCreator Find for protocol scheme:" << sch_ip_port.protocol;
    CHECK(false);
  }
  LOG_IF(ERROR, io_loops_.empty()) << __FUNCTION__ << " No loop handle socket io";

  CHECK(handler);
  message_handler_ = handler;
  CHECK(!io_loops_.empty());

  net::SocketAddr addr(sch_ip_port.host_ip, sch_ip_port.port);

  {
#if defined SO_REUSEPORT && defined NET_ENABLE_REUSER_PORT
    for (base::MessageLoop* loop : io_loops_) {
      IOServicePtr service(new IOService(addr, "http", loop, this));
      ioservices_.push_back(std::move(service));
    }
#else
    base::MessageLoop* loop = io_loops_[ioservices_.size() % io_loops_.size()];
    IOServicePtr service(new IOService(addr, "http", loop, this));
    ioservices_.push_back(std::move(service));
#endif

    for (IOServicePtr& service : ioservices_) {
      service->StartIOService();
    }
  }

}

void HttpServer::OnRequestMessage(const RefProtocolMessage& request) {
  VLOG(GLOG_VTRACE) << __FUNCTION__ << "a http request message come";

  HttpContext* context = new HttpContext(RefCast(HttpRequest, request));
  if (!dispatcher_) {
    return HandleHttpRequest(context);
  }
  base::StlClosure func = std::bind(&HttpServer::HandleHttpRequest, this, context);
  bool success = dispatcher_->Dispatch(func);
  if (!success) {
    context->ReplyString("Internal Server Error", 500);
    delete context;
    LOG(ERROR) << __FUNCTION__ << " HttpServer Internal Error, Dispach Request Failed";
  }
}

void HttpServer::HandleHttpRequest(HttpContext* context) {
  if (!dispatcher_) {
    context->Request()->SetWorkerCtx(base::MessageLoop::Current());
  } else {
    dispatcher_->SetWorkContext(context->Request());
  }

  message_handler_(context);

  if (!context->DidReply()) {
    context->ReplyString("Not Found", 404);
  }
  delete context;
}

bool HttpServer::CanCreateNewChannel() {
  return true;
}

base::MessageLoop* HttpServer::GetNextIOWorkLoop() {
#if defined SO_REUSEPORT && defined NET_ENABLE_REUSER_PORT
  return base::MessageLoop::Current();
#else
  static std::atomic_uint32_t index = 0;
  return io_loops_[index.fetch_add(1) % io_loops_.size()];
#endif
}

bool HttpServer::IncreaseChannelCount() {
  connection_count_.fetch_add(1);
  LOG(INFO) << __FUNCTION__ << " httpserver connections +1 count:" << connection_count_;
  return true;
}

void HttpServer::DecreaseChannelCount() {
  connection_count_.fetch_sub(1);
  LOG(INFO) << __FUNCTION__ << " httpserver connections -1 count:" << connection_count_;
}

bool HttpServer::BeforeIOServiceStart(IOService* ioservice) {
  return true;
}

void HttpServer::IOServiceStarted(const IOService* service) {
  LOG(INFO) << "Http Server IO Service " << service->IOServiceName() << " Started .......";

  { //sync
    std::unique_lock<std::mutex> lck(mtx_);
    for (auto& service : ioservices_) {
      if (!service->IsRunning()) return;
    }
    cv_.notify_all();
  }
}

void HttpServer::IOServiceStoped(const IOService* service) {
  LOG(INFO) << "Http Server IO Service " << service->IOServiceName() << " Stoped  .......";

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

void HttpServer::StopServerSync() {

  StopServer();

  std::unique_lock<std::mutex> lck(mtx_);
  while (cv_.wait_for(lck, std::chrono::milliseconds(500)) == std::cv_status::timeout) {
    LOG(INFO) << "stoping... ... ...";
  }
}

void HttpServer::StopServer() {
  for (auto& service : ioservices_) {
    service->StopIOService();
  }
}

}} //end net
