#ifndef NET_WORKLOAD_DISPATCHER_H
#define NET_WORKLOAD_DISPATCHER_H

#include <atomic>
#include <cinttypes>
#include <net_io/net_callback.h>
#include <net_io/protocol/proto_message.h>
#include <base/message_loop/message_loop.h>

namespace lt {
namespace net {

typedef std::vector<base::MessageLoop*> LoopList;

class Dispatcher {
public:
  Dispatcher(bool handle_in_io);
  virtual ~Dispatcher() {};

  bool HandleWorkInIOLoop() const {return handle_in_io_;}
  void SetWorkerLoops(LoopList& loops) {workers_ = loops;};

  // must call at Worker Loop, may ioworker
  // or woker according to HandleWorkInIOLoop
  virtual bool SetWorkContext(ProtocolMessage* message);
  // transmit task from IO TO worker loop
  virtual bool Dispatch(base::StlClosure& closuse);
protected:
  base::MessageLoop* NextWorker();
private:

  const bool handle_in_io_;
  std::vector<base::MessageLoop*> workers_;
  std::atomic<uint64_t> round_robin_counter_;
};

}}//end namespace net
#endif
