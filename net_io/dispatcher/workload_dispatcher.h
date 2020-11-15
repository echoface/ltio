#ifndef NET_WORKLOAD_DISPATCHER_H
#define NET_WORKLOAD_DISPATCHER_H

#include <atomic>
#include <cinttypes>
#include <net_io/net_callback.h>
#include <net_io/codec/codec_message.h>
#include <base/message_loop/message_loop.h>

namespace lt {
namespace net {

typedef std::vector<base::MessageLoop*> LoopList;

class Dispatcher {
public:
  Dispatcher(bool handle_in_io);
  virtual ~Dispatcher() {};

  void SetWorkerLoops(LoopList& loops) {workers_ = loops;};

  // must call at Worker Loop, may ioworker
  // or woker according to handle_in_io_ 
  virtual bool SetWorkContext(CodecMessage* message);
  // transmit task from IO TO worker loop
  virtual bool Dispatch(const base::LtClosure& closuse);
protected:
  const bool handle_in_io_;
  base::MessageLoop* NextWorker();

private:
  std::vector<base::MessageLoop*> workers_;
  std::atomic<uint64_t> round_robin_counter_;
};

}}//end namespace net
#endif
