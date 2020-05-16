#ifndef NET_HTTP_SERVER_H_H
#define NET_HTTP_SERVER_H_H

#include <list>
#include <vector>
#include <chrono>             // std::chrono::seconds
#include <functional>
#include <mutex>              // std::mutex, std::unique_lock
#include <condition_variable> // std::condition_variable, std::cv_status

#include "http_context.h"
#include "base/base_micro.h"
#include "base/message_loop/message_loop.h"
#include "net_io/io_service.h"
#include "net_io/codec/codec_message.h"
#include "net_io/codec/http/http_request.h"
#include "net_io/codec/http/http_response.h"

namespace lt {
namespace net {

typedef std::function<void(HttpContext* context)> HttpMessageHandler;

//?? use traits replace IOserviceDelegate override?
class HttpServer : public IOServiceDelegate {
public:
  HttpServer();
  ~HttpServer();

  void SetDispatcher(Dispatcher* dispatcher);
  void SetIOLoops(std::vector<base::MessageLoop*>& loops);

  void ServeAddress(const std::string, HttpMessageHandler);
  void StopServer();
  void SetCloseCallback(StlClosure callback);
protected:
  //override from ioservice delegate
  bool CanCreateNewChannel() override;
  void IncreaseChannelCount() override;
  void DecreaseChannelCount() override;
  base::MessageLoop* GetNextIOWorkLoop() override;
  void IOServiceStarted(const IOService* ioservice) override;
  void IOServiceStoped(const IOService* ioservice) override;
  void OnRequestMessage(const RefCodecMessage& request) override;

  // handle http request in target loop
  void HandleHttpRequest(HttpContext* context);

  base::MessageLoop* NextWorkerLoop();
private:
  Dispatcher* dispatcher_ = NULL;
  HttpMessageHandler message_handler_;

  std::mutex mtx_;
  std::condition_variable cv_;

  std::vector<base::MessageLoop*> io_loops_;
  std::list<RefIOService> ioservices_;
  StlClosure closed_callback_;

  std::atomic_bool serving_flag_;

  std::atomic_uint32_t connection_count_;

  std::atomic<uint32_t> worker_index_;
  DISALLOW_COPY_AND_ASSIGN(HttpServer);
};

}}
#endif
