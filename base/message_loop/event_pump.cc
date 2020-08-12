#include "fd_event.h"
#include "glog/logging.h"

#include "event_pump.h"
#include "io_multiplexer.h"
#include "io_mux_epoll.h"
#include "linux_signal.h"
#include <algorithm>
#include <sys/time.h>
#include <sys/resource.h>

namespace base {

EventPump::EventPump() : EventPump(NULL) {
}

EventPump::EventPump(PumpDelegate *d) :
  delegate_(d),
  running_(false) {

  struct rlimit limit;
  if (0 != getrlimit(RLIMIT_NOFILE, &limit)) {
    max_fds_ = 65535;
  } else {
    max_fds_ = limit.rlim_cur;
  }
  InitializeTimeWheel();
  io_mux_.reset(new base::IOMuxEpoll(max_fds_));
}

EventPump::~EventPump() {
  io_mux_.reset();
  if (timeout_wheel_) {
    FinalizeTimeWheel();
  }
}

void EventPump::Run() {
  IgnoreSigPipeSignalOnCurrentThread();

  running_ = true;
  if (delegate_) {
    delegate_->PumpStarted();
  }

  FiredEvent* active_list = new FiredEvent[max_fds_];
  while (running_) {

    int count = io_mux_->WaitingIO(active_list, NextTimeout());

    ProcessTimerEvent();

    InvokeFiredEvent(active_list, count);

    if (delegate_) {
      delegate_->RunNestedTask();
    }
  }
  running_ = false;
  delete []active_list;
  FinalizeTimeWheel();
  if (delegate_) {
    delegate_->PumpStopped();
  }
}

void EventPump::Quit() { running_ = false; }
bool EventPump::IsInLoopThread() const {
  return tid_ == std::this_thread::get_id();
}

bool EventPump::InstallFdEvent(FdEvent *fd_event) {
  CHECK(IsInLoopThread());
  if (fd_event->EventWatcher()) {
    LOG(ERROR) << __FUNCTION__ << " event has registered," << fd_event->EventInfo();
    return false;
  }
  fd_event->SetFdWatcher(io_mux_.get());
  io_mux_->AddFdEvent(fd_event);
  return true;
}

bool EventPump::RemoveFdEvent(FdEvent *fd_event) {
  CHECK(IsInLoopThread());
  if (!fd_event->EventWatcher()) {
    LOG(ERROR) << __FUNCTION__ << " event not been registered, " << fd_event->EventInfo();
    return false;
  }
  fd_event->SetFdWatcher(nullptr);

  io_mux_->DelFdEvent(fd_event);
  return true;
}

void EventPump::AddTimeoutEvent(TimeoutEvent *timeout_ev) {
  CHECK(IsInLoopThread());
  add_timer_internal(time_ms(), timeout_ev);
}

void EventPump::RemoveTimeoutEvent(TimeoutEvent *timeout_ev) {
  CHECK(IsInLoopThread());
  ::timeouts_del(timeout_wheel_, timeout_ev);
}

void EventPump::add_timer_internal(uint64_t now, TimeoutEvent *event) {
  ::timeouts_add(timeout_wheel_, event, now + event->Interval());
}

void EventPump::ProcessTimerEvent() {
  uint64_t now = time_ms();
  ::timeouts_update(timeout_wheel_, now);

  std::vector<TimeoutEvent *> to_be_deleted;

  Timeout *expired = NULL;
  while (NULL != (expired = timeouts_get(timeout_wheel_))) {
    // remove first
    ::timeouts_del(timeout_wheel_, expired);

    TimeoutEvent *timeout_ev = static_cast<TimeoutEvent *>(expired);

    if (timeout_ev->IsRepeated()) { // re-add to timeout wheel
      add_timer_internal(now, timeout_ev);
    } else if (timeout_ev->DelAfterInvoke()) {
      to_be_deleted.push_back(timeout_ev);
    }
    // Must at end; avoid case: ABA
    // timer A invoke -> delete A -> new A'(in same memory) -> free A'
    timeout_ev->Invoke();
  }

  for (auto toe : to_be_deleted) {
    delete toe;
  }
}

void EventPump::InvokeFiredEvent(FiredEvent* evs, int count) {
  for (int i = 0; i < count; i++) {
    FdEvent* fd_event = io_mux_->FindFdEvent(evs[i].fd_id);
    if (fd_event) {
      fd_event->HandleEvent(evs[i].event_mask);
    } else {
      LOG(ERROR) << __func__ << " event removed by previous handler";
    }
    evs[i].reset();
  }
}

timeout_t EventPump::NextTimeout() {
  static const uint64_t default_timeout_ms = 2000;

  ::timeouts_update(timeout_wheel_, time_ms());
  if (::timeouts_expired(timeout_wheel_)) {
    return 0;
  }

  timeout_t next_timeout = default_timeout_ms;
  if (::timeouts_pending(timeout_wheel_)) {
    next_timeout = ::timeouts_timeout(timeout_wheel_);
  }

  if (!delegate_) {
    return next_timeout;
  }
  return std::min(next_timeout, delegate_->PumpTimeout());
}

void EventPump::InitializeTimeWheel() {
  CHECK(timeout_wheel_ == NULL);

  int err = 0;
  timeout_wheel_ = ::timeouts_open(TIMEOUT_mHZ, &err);
  CHECK(err == 0);
  ::timeouts_update(timeout_wheel_, time_ms());
}

void EventPump::FinalizeTimeWheel() {
  CHECK(timeout_wheel_ != NULL);

  std::vector<TimeoutEvent *> to_be_delete;
  Timeout *to = NULL;
  TIMEOUTS_FOREACH(to, timeout_wheel_, TIMEOUTS_ALL) {
    TimeoutEvent *toe = static_cast<TimeoutEvent *>(to);
    ::timeouts_del(timeout_wheel_, to);
    if (toe->DelAfterInvoke()) {
      to_be_delete.push_back(toe);
    }
  }

  for (TimeoutEvent *toe : to_be_delete) {
    delete toe;
  }
  to_be_delete.clear();

  ::timeouts_close(timeout_wheel_);
  timeout_wheel_ = NULL;
}

} // namespace base
