#include "coro_dispatcher.h"

#include "glog/logging.h"
#include <base/base_constants.h>
#include <base/coroutine/coroutine_runner.h>

#include <net_io/tcp_channel.h>
#include <net_io/codec/codec_service.h>
#include <net_io/codec/codec_message.h>

namespace lt {
namespace net {

CoroDispatcher::CoroDispatcher(bool handle_in_io)
  : Dispatcher(handle_in_io) {
}

CoroDispatcher::~CoroDispatcher() {
}

bool CoroDispatcher::SetWorkContext(CodecMessage* message) {
  message->SetWorkerCtx(base::MessageLoop::Current(), co_resumer());
  return base::MessageLoop::Current();
}

bool CoroDispatcher::Dispatch(StlClosure closure) {
  if (handle_in_io_) {
    co_go closure;
  } else {
    co_go NextWorker() << closure;
  }
  return true;
}

}}//end namespace
