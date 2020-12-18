
#include "wait_group.h"
#include "coroutine_runner.h"
#include <base/message_loop/message_loop.h>

namespace base {

WaitGroup::WaitGroup()
  : wait_count_(0){
  flag_.clear();
}

WaitGroup::~WaitGroup() {
}

void WaitGroup::Done() {
  if (wait_count_.fetch_sub(1) != 1) {
    return;
  }
  wake_up();
}

void WaitGroup::Add(int64_t count) {
  CHECK(count > 0);
  wait_count_.fetch_add(count);
}

void WaitGroup::OnTimeOut() {
  timeouted_ = true;
  wake_up();
}

WaitGroup::WaitResult WaitGroup::Wait(int64_t timeout_ms) {
  if (flag_.test_and_set() || 0 == wait_count_.load()) {
    return kSuccess;
  }

  resumer_ = CO_RESUMER;

  MessageLoop* loop = MessageLoop::Current();
  if (timeout_ms != -1) {
    timeout_.reset(TimeoutEvent::CreateOneShot(timeout_ms, false));

    auto hdl = NewClosure(std::bind(&WaitGroup::OnTimeOut, this));
    timeout_->InstallTimerHandler(std::move(hdl));
    loop->Pump()->AddTimeoutEvent(timeout_.get());
  }

  CO_YIELD;

  if (timeout_ && !timeouted_) {
    loop->Pump()->RemoveTimeoutEvent(timeout_.get());
  }
  return timeouted_ ? kTimeout : kSuccess; 
}

} //end namespace base
