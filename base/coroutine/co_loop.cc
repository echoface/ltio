
#include "co_loop.h"
#include "base/closure/closure_task.h"
#include "base/message_loop/io_mux_epoll.h"
#include "base/message_loop/linux_signal.h"
#include "fcontext/fcontext.h"

namespace co {

CoLoop::CoLoop() {
  InitializeTimeWheel();

  io_mux_.reset(new base::IOMuxEpoll());
}

CoLoop::~CoLoop() {
  FinalizeTimeWheel();
}

bool CoLoop::PostTask(base::TaskBasePtr&& task) {
  runner_->ScheduleTask(std::move(task));
  return true;
}

bool CoLoop::PostDelayTask(base::TaskBasePtr&&, int64_t ms) {
  return false;
}

bool CoLoop::InLoop() const {
  return false;
}

timeout_t CoLoop::NextTimeout() {
  static const uint64_t default_timeout_ms = 50;
  if (runner_->HasPeedingTask()) {
    return 0;
  }
  ::timeouts_update(time_wheel_, base::time_ms());
  if (::timeouts_expired(time_wheel_)) {
    return 0;
  }
  timeout_t ms = default_timeout_ms;
  if (::timeouts_pending(time_wheel_)) {
    ms = std::min(ms, ::timeouts_timeout(time_wheel_));
  }
  return ms;
}

// static
void CoLoop::ThreadMain() {
  base::IgnoreSigPipeSignalOnCurrentThread();

  int ms = 0;
  std::vector<base::FiredEvent> active_list(65535);
  // init setup
  while (true) {
    ms = NextTimeout();
    int count = io_mux_->WaitingIO(active_list, ms);

    ProcessTimerEvent();

    InvokeFiredEvent(&active_list[0], count);

    runner_->Run();  // run schedule task
  }
  active_list.clear();
  // clean up
}

void CoLoop::InvokeFiredEvent(base::FiredEvent* evs, int count) {
  for (int i = 0; i < count; i++) {
    base::FiredEvent& ev = evs[i];
    auto func = [this, ev]() {
      base::FdEvent* fd_event = io_mux_->FindFdEvent(ev.fd_id);
      if (fd_event) {
        return fd_event->Invoke(ev.event_mask);
      }
      LOG(ERROR) << "event removed previously, fd:" << ev.fd_id;
    };
    runner_->AppendTask(NewClosure(std::move(func)));
    ev.reset();
  }
}

void CoLoop::ProcessTimerEvent() {
  ::timeouts_update(time_wheel_, base::time_ms());

  Timeout* expired = NULL;
  while (NULL != (expired = timeouts_get(time_wheel_))) {
    base::TimeoutEvent* timeout_ev = static_cast<base::TimeoutEvent*>(expired);

    if (timeout_ev->IsRepeated()) {
      runner_->AppendTask(NewClosure([timeout_ev]() { timeout_ev->Invoke(); }));
      continue;
    }

    // for oneshot timer, just pump/extract handler
    runner_->AppendTask(timeout_ev->ExtractHandler());
    if (timeout_ev->DelAfterInvoke()) {
      delete timeout_ev;
    }
  }
}

void CoLoop::InitializeTimeWheel() {
  CHECK(time_wheel_ == NULL);
  int err = 0;
  time_wheel_ = ::timeouts_open(TIMEOUT_mHZ, &err);
  CHECK(err == 0);
  ::timeouts_update(time_wheel_, base::time_ms());
}

void CoLoop::FinalizeTimeWheel() {
  CHECK(time_wheel_ != NULL);

  std::vector<base::TimeoutEvent*> to_be_delete;
  Timeout* to = NULL;
  TIMEOUTS_FOREACH(to, time_wheel_, TIMEOUTS_ALL) {
    base::TimeoutEvent* toe = static_cast<base::TimeoutEvent*>(to);
    ::timeouts_del(time_wheel_, to);
    if (toe->DelAfterInvoke()) {
      to_be_delete.push_back(toe);
    }
  }

  for (base::TimeoutEvent* toe : to_be_delete) {
    delete toe;
  }
  to_be_delete.clear();
  ::timeouts_close(time_wheel_);
  time_wheel_ = NULL;
}

}  // namespace co
