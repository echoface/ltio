
#include "wait_group.h"
#include "coroutine_runner.h"
#include <base/message_loop/message_loop.h>

namespace base {

WaitGroup::WaitGroup()
  : timeout_(NULL) {
  flag_.clear();
  wakeup_flag_.clear();
  wait_count_.store(0);
}

WaitGroup::~WaitGroup() {
}

void WaitGroup::Done() {
  if (wait_count_.fetch_sub(1) == 1) {
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

  resumer_ = co_resumer();
  MessageLoop* loop = MessageLoop::Current();
  if (timeout_ms != -1) {
    timeout_ = TimeoutEvent::CreateOneShot(timeout_ms, false);
    timeout_->InstallTimerHandler(NewClosure(std::bind(&WaitGroup::wake_up, this)));
    loop->Pump()->AddTimeoutEvent(timeout_);
  }

  co_pause;

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
  resumer_();
}

}
