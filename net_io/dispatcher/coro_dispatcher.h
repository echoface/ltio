#ifndef NET_COROWORKLOAD_DISPATCHER_H_H
#define NET_COROWORKLOAD_DISPATCHER_H_H

#include "workload_dispatcher.h"
#include <base/closure/closure_task.h>

namespace lt {
namespace net {

class CoroDispatcher : public Dispatcher {
public:
  CoroDispatcher(bool handle_in_io);
  ~CoroDispatcher();

  bool Dispatch(const base::LtClosure& clourse) override;
  bool SetWorkContext(CodecMessage* message) override;
};

}}
#endif
