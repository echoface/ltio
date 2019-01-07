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

CoroWlDispatcher::CoroWlDispatcher(bool handle_in_io)
  : WorkLoadDispatcher(handle_in_io) {
}

CoroWlDispatcher::~CoroWlDispatcher() {
}

void CoroWlDispatcher::TransferAndYield(base::MessageLoop* ioloop, base::StlClosure clourse) {
  CHECK(ioloop);
  ioloop->PostTask(NewClosure(std::move(clourse)));
  co_yield;
}

bool CoroWlDispatcher::ResumeWorkContext(WorkContext& ctx) {
	if (!ctx.resume_ctx) {
	  return false;
	}
	ctx.resume_ctx();
	return true;
}

bool CoroWlDispatcher::SetWorkContext(WorkContext& ctx) {
  LOG(INFO) << " seems loop current is null:" << base::MessageLoop::Current();
  LOG(INFO) << " seems loop current can't yield:" << base::CoroRunner::CanYield();
  if (base::MessageLoop::Current() && base::CoroRunner::CanYield()) {
    ctx.loop = base::MessageLoop::Current();
    ctx.resume_ctx = co_resumer; //base::CoroRunner::CurrentCoroResumeCtx();
    return true;
  }
  return false;
}

bool CoroWlDispatcher::TransmitToWorker(base::StlClosure& closure) {
  base::MessageLoop* loop = HandleWorkInIOLoop() ? base::MessageLoop::Current() : GetNextWorkLoop();

  if (NULL == loop) {
    LOG(ERROR) << "no message loop handler this task";
    return false;
  }

  if (loop->IsInLoopThread()) {
    co_go closure;
  } else {
    co_go loop << closure;
  }
  return true;
}

}//end namespace
