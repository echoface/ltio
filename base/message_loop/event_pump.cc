#include "fd_event.h"
#include "glog/logging.h"
#include <base/utils/sys_error.h>

#include "event_pump.h"
#include "io_mux_epoll.h"
#include "linux_signal.h"
#include <algorithm>
#include <iostream>

namespace base {

EventPump::EventPump() :
  delegate_(NULL),
  running_(false) {

  InitializeTimeWheel();
  io_mux_.reset(new base::IOMuxEpoll());
}

EventPump::EventPump(PumpDelegate *d) :
  delegate_(d),
  running_(false) {

  InitializeTimeWheel();
  io_mux_.reset(new base::IOMuxEpoll());
}

EventPump::~EventPump() {
  io_mux_.reset();
  if (timeout_wheel_) {
    FinalizeTimeWheel();
  }
}

void EventPump::Run() {
  static const uint64_t default_timeout_ms = 2000;

  IgnoreSigPipeSignalOnCurrentThread();

  uint64_t perfect_timeout_ms = 0;
  std::vector<FdEvent *> active_events;

  running_ = true;
  if (delegate_) {
    delegate_->PumpStarted();
  }
  while (running_) {
    active_events.clear();

    io_mux_->WaitingIO(active_events, NextTimeout(default_timeout_ms));

    ProcessTimerEvent();

    for (FdEvent* fd_event : active_events) {
      fd_event->HandleEvent();
    }

    if (delegate_) {
      delegate_->RunNestedTask();
    }
  }
  FinalizeTimeWheel();
  running_ = false;
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

  fd_event->SetFdWatcher(AsFdWatcher());
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

void EventPump::OnEventChanged(FdEvent *fd_event) {
  CHECK(IsInLoopThread() && fd_event->EventWatcher());
  io_mux_->UpdateFdEvent(fd_event);
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

timeout_t EventPump::NextTimeout(timeout_t hint) {
  ::timeouts_update(timeout_wheel_, time_ms());
  if (::timeouts_expired(timeout_wheel_)) {
    return 0;
  }

  if (::timeouts_pending(timeout_wheel_)) {
    hint = ::timeouts_timeout(timeout_wheel_);
  }

  if (!delegate_) {
    return hint;
  }
  return std::min(hint, delegate_->PumpTimeout());
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
