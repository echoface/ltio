/*
 * Copyright 2021 <name of copyright holder>
 * Author: Huan.Gong <gonghuan.dev@gmail.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <sys/resource.h>
#include <sys/time.h>
#include <algorithm>

#include "base/base_constants.h"
#include "event_pump.h"
#include "fd_event.h"
#include "io_multiplexer.h"
#include "io_mux_epoll.h"
#include "linux_signal.h"

#include "glog/logging.h"

namespace base {

namespace {

thread_local uint64_t tid_ = 0;  // assign with current run loop

}

// static
uint64_t EventPump::CurrentThreadLoopID() {
  return tid_;
}

EventPump::EventPump() : fired_list_(65535) {
  InitializeTimeWheel();
  io_mux_.reset(new base::IOMuxEpoll());
}

EventPump::~EventPump() {
  io_mux_.reset();
  if (timeout_wheel_) {
    FinalizeTimeWheel();
  }
}

bool EventPump::IsInLoop() const {
  return loop_id_ == tid_ && loop_id_ > 0;
}

void EventPump::SetLoopId(uint64_t id) {
  loop_id_ = id;
}

void EventPump::PrepareRun() {
  tid_ = loop_id_;
  CHECK(loop_id_);
  IgnoreSigPipeSignalOnCurrentThread();
}

void EventPump::Pump(uint64_t ms) {
  CHECK(IsInLoop());

  ms = std::min(ms, NextTimeout());
  VLOG_EVERY_N(GLOG_VTRACE, 1000) << " poll timeout:" << ms;

  int count = io_mux_->WaitingIO(fired_list_, ms);

  ProcessTimerEvent();

  InvokeFiredEvent(&fired_list_[0], count);
}

bool EventPump::InstallFdEvent(FdEvent* fd_event) {
  CHECK(IsInLoop());
  if (fd_event->EventWatcher()) {
    LOG(ERROR) << "event has registered," << fd_event->EventInfo();
    return false;
  }
  io_mux_->AddFdEvent(fd_event);
  return true;
}

bool EventPump::RemoveFdEvent(FdEvent* fd_event) {
  CHECK(IsInLoop());

  if (!fd_event->EventWatcher()) {
    LOG(ERROR) << "event not been registered, " << fd_event->EventInfo();
    return false;
  }
  io_mux_->DelFdEvent(fd_event);
  return true;
}

void EventPump::AddTimeoutEvent(TimeoutEvent* timeout_ev) {
  CHECK(IsInLoop());

  add_timer_internal(time_ms(), timeout_ev);
}

void EventPump::RemoveTimeoutEvent(TimeoutEvent* timeout_ev) {
  CHECK(IsInLoop());

  ::timeouts_del(timeout_wheel_, timeout_ev);
}

void EventPump::add_timer_internal(uint64_t now, TimeoutEvent* event) {
  timeout_t t =
      event->IsRepeated() ? event->Interval() : now + event->Interval();
  ::timeouts_add(timeout_wheel_, event, t);
}

void EventPump::ProcessTimerEvent() {
  ::timeouts_update(timeout_wheel_, time_ms());

  Timeout* expired = NULL;
  while (NULL != (expired = timeouts_get(timeout_wheel_))) {
    TimeoutEvent* timeout_ev = static_cast<TimeoutEvent*>(expired);

    timeout_ev->Invoke();

    if (!timeout_ev->IsRepeated() && timeout_ev->DelAfterInvoke()) {
      delete timeout_ev;
    }
  }
}

void EventPump::InvokeFiredEvent(FiredEvent* evs, int count) {
  for (int i = 0; i < count; i++) {
    FdEvent* fd_event = io_mux_->FindFdEvent(evs[i].fd_id);
    if (fd_event) {
      fd_event->Invoke(evs[i].event_mask);
    } else {
      LOG(ERROR) << " event removed by previous handler";
    }
    evs[i].reset();
  }
}

timeout_t EventPump::NextTimeout() {
  static const uint64_t default_timeout_ms = 50;

  ::timeouts_update(timeout_wheel_, time_ms());
  if (::timeouts_expired(timeout_wheel_)) {
    return 0;
  }

  timeout_t next_timeout = default_timeout_ms;
  if (::timeouts_pending(timeout_wheel_)) {
    next_timeout = std::min(next_timeout, ::timeouts_timeout(timeout_wheel_));
  }
  return next_timeout;
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

  std::vector<TimeoutEvent*> to_be_delete;
  Timeout* to = NULL;
  TIMEOUTS_FOREACH(to, timeout_wheel_, TIMEOUTS_ALL) {
    TimeoutEvent* toe = static_cast<TimeoutEvent*>(to);
    ::timeouts_del(timeout_wheel_, to);
    if (toe->DelAfterInvoke()) {
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

}  // namespace base
