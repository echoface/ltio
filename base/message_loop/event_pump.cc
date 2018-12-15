#include "event_pump.h"

#include "glog/logging.h"
#include "linux_signal.h"
#include "io_multiplexer_epoll.h"

namespace base {

EventPump::EventPump()
  : delegate_(NULL),
    running_(false) {
  multiplexer_.reset(new base::IoMultiplexerEpoll());
}

EventPump::EventPump(PumpDelegate* d)
  : delegate_(d),
    running_(false) {
  multiplexer_.reset(new base::IoMultiplexerEpoll());
}

EventPump::~EventPump() {
  multiplexer_.reset();
  if (timeout_wheel_) {
    FinalizeTimeWheel();
  }
}

void EventPump::Run() {
  //sigpipe
  Signal::signal(SIGPIPE, []() { LOG(ERROR) << "sigpipe."; });

  if (delegate_) {
    delegate_->BeforePumpRun();
  }

  running_ = true;
  InitializeTimeWheel();

  std::vector<FdEvent*> active_events;
  static uint64_t default_epoll_timeout_us = 2000000;

  while (running_) {
    active_events.clear();
  
    uint64_t perfect_timeout_us = NextTimerTimeout(default_epoll_timeout_us);

    multiplexer_->WaitingIO(active_events, perfect_timeout_us/1000.0);

    ProcessTimerEvent();

    for (auto& fd_event : active_events) {
      fd_event->HandleEvent();
    }

    if (delegate_) {
      delegate_->RunNestedTask();
    }
  }
  running_ = false;
  FinalizeTimeWheel();

  if (delegate_) {
    delegate_->AfterPumpRun();
  }
}

void EventPump::Quit() {
  running_ = false;
}

bool EventPump::IsInLoopThread() const {
  return tid_ == std::this_thread::get_id();
}

QuitClourse EventPump::Quit_Clourse() {
  return std::bind(&EventPump::Quit, this);
}

bool EventPump::InstallFdEvent(FdEvent *fd_event) {
  CHECK(IsInLoopThread());
  if (fd_event->EventWatcher()) {
    LOG(ERROR) << __FUNCTION__ << " event has has monitored";
    return false;
  }

  fd_event->SetFdWatcher(AsFdWatcher());
  multiplexer_->AddFdEvent(fd_event);
  return true;
}

bool EventPump::RemoveFdEvent(FdEvent* fd_event) {
  CHECK(IsInLoopThread());
  if (!fd_event->EventWatcher()) {
    LOG(ERROR) << __FUNCTION__ << " event don't attach to any Pump";
    return false;
  }
  fd_event->SetFdWatcher(nullptr);

  multiplexer_->DelFdEvent(fd_event);
  return true;
}

void EventPump::OnEventChanged(FdEvent* fd_event) {
  CHECK(IsInLoopThread() && fd_event->EventWatcher());
  multiplexer_->UpdateFdEvent(fd_event);
}

void EventPump::AddTimeoutEvent(TimeoutEvent* timeout_ev) {
  CHECK(IsInLoopThread());
  add_timer_internal(time_us(), timeout_ev);
}

void EventPump::RemoveTimeoutEvent(TimeoutEvent* timeout_ev) {
  CHECK(IsInLoopThread());
  ::timeouts_del(timeout_wheel_, timeout_ev); 
}

void EventPump::add_timer_internal(uint64_t now_us, TimeoutEvent* event) {
  ::timeouts_add(timeout_wheel_, event, now_us + event->IntervalMicroSecond());
}

void EventPump::ProcessTimerEvent() {
  uint64_t now = time_us();
  ::timeouts_update(timeout_wheel_, now);

	Timeout* expired = NULL;
  while (NULL != (expired = timeouts_get(timeout_wheel_))) {

    TimeoutEvent* timeout_ev = static_cast<TimeoutEvent*>(expired);
    timeout_ev->InvokeTimerHanlder();

    if (timeout_ev->IsRepeated()) { // re-add to timeout wheel

      add_timer_internal(now, timeout_ev);

    } else {

      ::timeouts_del(timeout_wheel_, expired);
      if (timeout_ev->SelfDelete()) {
        delete timeout_ev;
      }
    }

  } // end while
}

timeout_t EventPump::NextTimerTimeout(timeout_t default_timeout) {

  ::timeouts_update(timeout_wheel_, time_us());

  if (::timeouts_expired(timeout_wheel_)) {
    return 0;
  }
  if (::timeouts_pending(timeout_wheel_)) {
    return ::timeouts_timeout(timeout_wheel_);
  }
  return default_timeout;
}

void EventPump::InitializeTimeWheel() {
  int err = 0;
  timeout_wheel_ = ::timeouts_open(TIMEOUT_uHZ, &err); 
  ::timeouts_update(timeout_wheel_, time_us()); 
}

void EventPump::FinalizeTimeWheel() {

  std::vector<TimeoutEvent*> to_be_delete;

  Timeout* to = NULL;
  TIMEOUTS_FOREACH(to, timeout_wheel_, TIMEOUTS_ALL) {
    TimeoutEvent* toe = static_cast<TimeoutEvent*>(to);

    ::timeouts_del(timeout_wheel_, to);
    if (toe->SelfDelete()) {
      to_be_delete.push_back(toe);
    }
  }
  
  for (TimeoutEvent* toe : to_be_delete) {
    delete toe;
  }
  to_be_delete.clear();

  ::timeouts_close(timeout_wheel_);
  timeout_wheel_ = NULL;
}

}//end base
