#ifndef NET_SERVER_BASE_H_H
#define NET_SERVER_BASE_H_H

#include <chrono>              // std::chrono::seconds
#include <condition_variable>  // std::condition_variable, std::cv_status
#include <functional>
#include <list>
#include <mutex>  // std::mutex, std::unique_lock
#include <vector>

#include "base/base_micro.h"
#include "base/message_loop/message_loop.h"
#include "io_service.h"
#include "protocol/proto_message.h"

namespace lt {
namespace net {

typedef std::list<IOServicePtr> IOServiceList;
typedef std::vector<base::MessageLoop*> LoopList;
class ServerBase : public IOServiceDelegate {
 public:
   ServerBase();
   ~ServerBase();

   ServerBase* WithMaxConnection(uint32_t limit);
   ServerBase* WithDidspatcher(Dispatcher* dispatcher);
   ServerBase* WithIOLoops(LoopList& loops);
  protected:
   void Serve(const std::string& server_adress);

   //override from ioservice delegate
   bool CanCreateNewChannel() override;
   bool IncreaseChannelCount() override;
   void DecreaseChannelCount() override;
   base::MessageLoop* GetNextIOWorkLoop() override;
   bool BeforeIOServiceStart(IOService* ioservice) override;
   void IOServiceStarted(const IOService* ioservice) override;
   void IOServiceStoped(const IOService* ioservice) override;
   void OnRequestMessage(const RefProtocolMessage&) override;

   std::mutex mtx_;
   std::condition_variable cv_;

   uint32_t max_connections_ = 0; //0: no limit
   Dispatcher* dispatcher_ = NULL;

   LoopList io_loops_;
   IOServiceList ioservices_;

   std::atomic<bool> serving_flag_;
   std::atomic<uint32_t> connection_count_;
   DISALLOW_COPY_AND_ASSIGN(RawServer);
};

}  // namespace net
}  // namespace lt
