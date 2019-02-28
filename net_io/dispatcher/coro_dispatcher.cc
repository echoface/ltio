#include "glog/logging.h"

#include "tcp_channel.h"
#include "coro_dispatcher.h"
#include "protocol/proto_service.h"
#include "protocol/proto_message.h"

#include "base/base_constants.h"
#include "base/coroutine/coroutine_task.h"
#include "base/coroutine/coroutine.h"
#include "base/coroutine/coroutine_runner.h"

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

}//end namespace
