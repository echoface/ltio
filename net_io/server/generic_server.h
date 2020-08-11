#ifndef LT_NET_GENERIC_SERVER_H_H
#define LT_NET_GENERIC_SERVER_H_H

#include <cstdint>
#include <list>
#include <memory>
#include <mutex>              // std::mutex, std::unique_lock
#include <vector>
#include <chrono>             // std::chrono::seconds
#include <functional>

#include "base/base_micro.h"
#include "net_io/io_service.h"
#include "net_io/codec/codec_factory.h"
#include "base/message_loop/message_loop.h"

namespace lt {
namespace net {

using base::MessageLoop;

typedef struct DefaultConfigurator {
  static const uint64_t kClientConnLimit = 65536;
  static const uint64_t kRequestQpsLimit = 100000;
}DefaultConfigurator;

template<class Context, class T>
class BaseServer : public IOServiceDelegate {
public:
  typedef BaseServer<Context, T> Server;
  typedef std::shared_ptr<Context> ContextPtr;
  typedef std::vector<MessageLoop*> MessageLoopList;
  typedef std::list<RefIOService> RefIOServiceList;
  typedef std::function<void(ContextPtr context)> Handler;

  BaseServer()
    : dispatcher_(nullptr),
      serving_flag_(false),
      client_count_(0) {
  }

  Server& WithIOLoops(MessageLoopList& loops) {
    io_loops_ = loops;
    return *this;
  }
  Server& WithDispatcher(Dispatcher* dispatcher) {
    dispatcher_ = dispatcher;
    return *this;
  };
  const IPEndPoint& Endpoint() const {
    return endpoint_;
  };
  const url::SchemeIpPort& ServeURI() const {
    return uri_;
  }

  void ServeAddress(const std::string& address, Handler&& handler) {
    bool served = serving_flag_.exchange(true);
    CHECK(!served);
    CHECK(handler);
    CHECK(!io_loops_.empty());

    handler_ = handler;

    if (!url::ParseURI(address, uri_)) {
      LOG(ERROR) << "uri parse error,eg:[raw://xx.xx.xx.xx:port]";
      CHECK(false);
    }
    if (!CodecFactory::HasCreator(uri_.protocol)) {
      LOG(ERROR) << "scheme:" << uri_.protocol << " codec not found";
      CHECK(false);
    }

    endpoint_ = net::IPEndPoint(uri_.host_ip, uri_.port);
#if defined SO_REUSEPORT && defined NET_ENABLE_REUSER_PORT
    for (base::MessageLoop* loop : io_loops_) {
      RefIOService service(new IOService(endpoint_, uri_.protocol, loop, this));
      service->StartIOService();
      ioservices_.push_back(std::move(service));
    }
#else
    base::MessageLoop* loop = io_loops_.front();
    RefIOService service(new IOService(addr, uri_.protocol, loop, this));
    service->StartIOService();
    ioservices_.push_back(std::move(service));
#endif
  }

  void StopServer(ClosureCallback callback = nullptr) {
    CHECK(serving_flag_);
    closed_callback_ = callback;

    std::list<RefIOService> services = ioservices_;
    for (RefIOService& service : services) {
      base::MessageLoop* io = service->AcceptorLoop();
      io->PostTask(FROM_HERE, &IOService::StopIOService, service);
    }
  }
protected:
  //override from ioservice delegate
  bool CanCreateNewChannel() override {
    bool can = client_count_ < T::kClientConnLimit;
    LOG_IF(INFO, !can) << " max client limit reach";
    return can;
  };

  void IncreaseChannelCount() override { client_count_++;}

  void DecreaseChannelCount() override { client_count_--;}

  MessageLoop* GetNextIOWorkLoop() override {
#if defined SO_REUSEPORT && defined NET_ENABLE_REUSER_PORT
    return MessageLoop::Current();
#else
    static std::atomic_uint32_t index = 0;
    return io_loops_[index.fetch_add(1) % io_loops_.size()];
#endif
  };

  void IOServiceStarted(const IOService* service) {
    std::unique_lock<std::mutex> lck(mtx_);
    for (const RefIOService& io_service : ioservices_) {
      if (!io_service->IsRunning()) {
        return;
      }
    }
    LOG(INFO) << "Server " << ServerInfo() << " Started";
  }

  void IOServiceStoped(const IOService* service) override {
    std::unique_lock<std::mutex> lck(mtx_);
    ioservices_.remove_if([&](const RefIOService& s) -> bool {
      return s.get() == service;
    });

    LOG_IF(INFO, ioservices_.empty()) << "Server:" << ServerInfo() << " Stoped";
    if (ioservices_.empty() && closed_callback_) {
      closed_callback_();
    }
  }

  void OnRequestMessage(const RefCodecMessage& request) override {
    if (!dispatcher_) {
      this->handler_(std::make_shared<Context>(std::move(request)));
    }

    bool success = dispatcher_->Dispatch([=](){
      this->handler_(std::make_shared<Context>(std::move(request)));
    });
    CHECK(success);
  }

  std::string ServerInfo() const {
    return uri_.protocol + ":" + endpoint_.ToString();
  }
private:
  Handler handler_;
  Dispatcher* dispatcher_;

  std::mutex mtx_;
  IPEndPoint endpoint_;
  url::SchemeIpPort uri_;
  MessageLoopList io_loops_;
  RefIOServiceList ioservices_;

  ClosureCallback closed_callback_;
  std::atomic<bool> serving_flag_;
  std::atomic_uint32_t client_count_;
  DISALLOW_COPY_AND_ASSIGN(BaseServer);
};

}}
#endif

