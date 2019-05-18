#include "timeout_event.h"

namespace base {

//static
TimeoutEvent* TimeoutEvent::CreateOneShot(uint64_t ms, bool delelte_after_invoke) {
  TimeoutEvent* toe = new TimeoutEvent(ms, false);
  toe->del_after_invoke_ = delelte_after_invoke;
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
