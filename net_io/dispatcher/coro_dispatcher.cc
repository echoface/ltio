#include "coro_dispatcher.h"

#include "glog/logging.h"
#include <base/base_constants.h>
#include <base/coroutine/coroutine.h>
#include <base/coroutine/coroutine_runner.h>

#include <net_io/tcp_channel.h>
#include <net_io/protocol/proto_service.h>
#include <net_io/protocol/proto_message.h>

namespace lt {
namespace net {

CoroDispatcher::CoroDispatcher(bool handle_in_io)
  : Dispatcher(handle_in_io) {
}

CoroDispatcher::~CoroDispatcher() {
}

void CoroDispatcher::TransferAndYield(base::MessageLoop* ioloop, base::StlClosure clourse) {
  CHECK(ioloop);
  ioloop->PostTask(NewClosure(std::move(clourse)));
  co_yield;
}

bool CoroDispatcher::SetWorkContext(ProtocolMessage* message) {
  //base::CoroRunner::CurrentCoroResumeCtx();
  message->SetWorkerCtx(base::MessageLoop::Current(), co_resumer);
  return base::MessageLoop::Current() && base::CoroRunner::CanYield();
}

bool CoroDispatcher::Dispatch(base::StlClosure& closure) {

  if (HandleWorkInIOLoop()) {
    co_go closure;
    return true;
  }
  co_go NextWorker() << closure;
  return true;
}

}}//end namespace
