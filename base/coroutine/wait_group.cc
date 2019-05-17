
#include "wait_group.h"
#include "coroutine_runner.h"
#include <base/message_loop/message_loop.h>

namespace base {

WaitGroup::WaitGroup()
  : timeout_(NULL),
    wait_count_(ATOMIC_VAR_INIT(0)) {
  flag_.clear();
  wakeup_flag_.clear();
}

WaitGroup::~WaitGroup() {
}

void WaitGroup::Done() {
  int64_t pre_count = wait_count_.fetch_sub(1);
  CHECK(pre_count > 0);

  if (pre_count == 1) {
    wake_up();
  }
}

void WaitGroup::Add(int64_t count) {
  CHECK(count > 0);
  wait_count_ += count;
}

void WaitGroup::Wait(int64_t timeout_ms) {
  if (flag_.test_and_set()) {
    return;
  }

  resumer_ = co_resumer;
  base::MessageLoop* loop = base::MessageLoop::Current();
  if (timeout_ms != -1) {
    timeout_ = TimeoutEvent::CreateOneShotTimer(timeout_ms, false);
    loop->Pump()->AddTimeoutEvent(timeout_);
  }

  co_yield;

  if (timeout_) {
    loop->Pump()->RemoveTimeoutEvent(timeout_);
    delete timeout_;
    timeout_ = NULL;
  }
}

void WaitGroup::wake_up() {//timeout or done
  if (wakeup_flag_.test_and_set()) {
    return;
  }
  CHECK(resumer_);
  resumer_();
  resumer_ = nullptr;
}

}
