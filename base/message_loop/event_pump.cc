#include "event_pump.h"

#include "glog/logging.h"
#include "linux_signal.h"
#include "io_multiplexer_epoll.h"

namespace base {

EventPump::EventPump()
  : delegate_(NULL),
    running_(false) {
  // fd iomux
  multiplexer_.reset(new base::IoMultiplexerEpoll());

  // timer
  int err = 0;
  timeout_wheel_ = ::timeouts_open(TIMEOUT_mHZ, &err); 
  ::timeouts_update(timeout_wheel_, time_ms()); 

}

EventPump::EventPump(PumpDelegate* d)
  : delegate_(d),
    running_(false) {
  multiplexer_.reset(new base::IoMultiplexerEpoll());
}

EventPump::~EventPump() {
  multiplexer_.reset();
}

void EventPump::Run() {
  //sigpipe
  Signal::signal(SIGPIPE, []() { LOG(ERROR) << "sigpipe."; });

  if (delegate_) {
    delegate_->BeforePumpRun();
  }
  running_ = true;
  while (running_) {

    uint64_t perfect_timeout = timer_queue_.HandleExpiredTimer();

    active_events_.clear();

    perfect_timeout = std::min(perfect_timeout, NextTimerTimeoutms(perfect_timeout));

    multiplexer_->WaitingIO(active_events_, perfect_timeout);

    ProcessTimerEvent();

    for (auto& fd_event : active_events_) {
      fd_event->HandleEvent();
    }

    if (delegate_) {
      delegate_->RunNestedTask();
    }
  }

  running_ = false;

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
  if (fd_event->EventDelegate()) {
    LOG(ERROR) << "can't install event to eventPump twice or install event to two different eventPump";
    return false;
  }

  fd_event->SetDelegate(AsFdWatcher());

  multiplexer_->AddFdEvent(fd_event);
  return true;
}

bool EventPump::RemoveFdEvent(FdEvent* fd_event) {
  CHECK(IsInLoopThread());
  if (!fd_event->EventDelegate()) {
    LOG(ERROR) << "event don't attach to any Pump";
    return false;
  }
  fd_event->SetDelegate(nullptr);

  multiplexer_->DelFdEvent(fd_event);
  return true;
}

void EventPump::OnEventChanged(FdEvent* fd_event) {
  CHECK(IsInLoopThread() && fd_event->EventDelegate());
  multiplexer_->UpdateFdEvent(fd_event);
}

int32_t EventPump::ScheduleTimer(RefTimerEvent& timerevent) {
  return timer_queue_.AddTimerEvent(timerevent);
}

bool EventPump::CancelTimer(uint32_t timer_id) {
  CHECK(IsInLoopThread());
  return timer_queue_.CancelTimerEvent(timer_id);
}

void EventPump::AddTimeoutEvent(TimeoutEvent* timeout_ev) {
  CHECK(IsInLoopThread());
  add_timer_internal(timeout_ev);
}

void EventPump::RemoveTimeoutEvent(TimeoutEvent* timeout_ev) {
  CHECK(IsInLoopThread());
  ::timeouts_del(timeout_wheel_, timeout_ev); 
}

void EventPump::add_timer_internal(TimeoutEvent* event) {
  ::timeouts_add(timeout_wheel_, event, time_ms() + event->Interval());
}

void EventPump::ProcessTimerEvent() {
  uint64_t now = time_ms();
  ::timeouts_update(timeout_wheel_, now);

	Timeout* expired = NULL;
  while (NULL != (expired = timeouts_get(timeout_wheel_))) {
    TimeoutEvent* timeout_ev = static_cast<TimeoutEvent*>(expired);
    timeout_ev->InvokeTimerHanlder();
    if (timeout_ev->IsRepeated()) { // readd to timeout wheel
      add_timer_internal(timeout_ev);
    } else {
      ::timeouts_del(timeout_wheel_, expired);
      if (timeout_ev->SelfDelete()) {
        delete timeout_ev;   
      }
    }
  }
}

timeout_t EventPump::NextTimerTimeoutms(timeout_t default_timeout) {

  ::timeouts_update(timeout_wheel_, time_ms());

  if (::timeouts_expired(timeout_wheel_)) {
    return 0;
  }
  if (::timeouts_pending(timeout_wheel_)) {
    return ::timeouts_timeout(timeout_wheel_);
  }
  return default_timeout;
}


}//end base
