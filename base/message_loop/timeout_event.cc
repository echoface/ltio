#include "timeout_event.h"

namespace base {
  
//static  
TimeoutEvent* TimeoutEvent::CreateSelfDeleteTimeoutEvent(uint64_t ms) {
  TimeoutEvent* toe = new TimeoutEvent(ms, false);
  toe->self_delete_ = true;
  return toe;
}

TimeoutEvent::TimeoutEvent(uint64_t ms, bool repeat) :
  repeat_(repeat),
  interval_(ms) {
  ::timeout_init((Timeout*)this, TIMEOUT_ABS);
}

TimeoutEvent::~TimeoutEvent() {
  CHECK(!IsAttached());
}

void TimeoutEvent::UpdateInterval(int64_t ms) {
  interval_ = ms;
}

void TimeoutEvent::Invoke() {
  if (timer_handler_) {
    timer_handler_->Run();
  }
}

void TimeoutEvent::InstallTimerHandler(ClosurePtr&& h) {
  timer_handler_ = std::move(h);
}

}
