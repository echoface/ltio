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
  resumer_();
}

void WaitGroup::Add(int64_t count) {
  CHECK(count > 0);
  wait_count_.fetch_add(count);
}

WaitGroup::WaitResult WaitGroup::Wait(int64_t timeout_ms) {
  if (flag_.test_and_set() || 0 == wait_count_.load()) {
    return kSuccess;
  }

  resumer_ = CO_RESUMER;

  MessageLoop* loop = MessageLoop::Current();
  if (timeout_ms != -1) {
    timeout_.reset(TimeoutEvent::CreateOneShot(timeout_ms, false));

    timeout_->InstallTimerHandler(NewClosure(resumer_));
    loop->Pump()->AddTimeoutEvent(timeout_.get());
  }

  CO_YIELD;

  if (timeout_) {
    loop->Pump()->RemoveTimeoutEvent(timeout_.get());
  }
  return kSuccess;
}

} //end namespace base
