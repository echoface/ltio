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

#include "repeating_timer.h"

#include <glog/logging.h>

#include <base/memory/scoped_guard.h>

namespace base {

// new a timer with spcial runner loop
RepeatingTimer::RepeatingTimer(MessageLoop* loop)
  : running_(false),
    runner_loop_(loop) {
  CHECK(runner_loop_);
}

RepeatingTimer::~RepeatingTimer() {
  if (running_) {
    Stop();
  }
}

bool RepeatingTimer::Running() const {
  return running_;
}

void RepeatingTimer::Start(uint64_t ms, LtClosure user_task) {
  if (running_) {
    Stop();
  }

  CHECK(!timeout_event_);

  timeout_event_.reset(new TimeoutEvent(ms, true));
  timeout_event_->InstallHandler(
      NewClosure(std::bind(&RepeatingTimer::OnTimeout, this)));
  user_task_ = std::move(user_task);

  ScopedGuard running([this]() { this->running_ = true; });

  if (runner_loop_->IsInLoopThread()) {
    return runner_loop_->Pump()->AddTimeoutEvent(timeout_event_.get());
  }

  runner_loop_->PostTask(FROM_HERE,
                         &EventPump::AddTimeoutEvent,
                         runner_loop_->Pump(),
                         timeout_event_.get());
}

// reset timeoutevent and reset it
void RepeatingTimer::Stop() {
  ScopedGuard running([this]() { this->running_ = false; });

  if (runner_loop_->IsInLoopThread() && timeout_event_.get()) {
    runner_loop_->Pump()->RemoveTimeoutEvent(timeout_event_.get());
    timeout_event_.reset();
    return;
  }

  bool removed = false;
  auto remover_fn = std::bind(
      [&](EventPump* pump, TimeoutEvent* timeout_event) {
        pump->RemoveTimeoutEvent(timeout_event_.get());
        timeout_event_.reset();
        removed = true;
        cv.notify_all();
      },
      runner_loop_->Pump(),
      timeout_event_.get());

  runner_loop_->PostTask(NewClosure(remover_fn));

  {
    std::unique_lock<std::mutex> lk(m);
    cv.wait(lk, [&] { return removed; });
  }
}

void RepeatingTimer::OnTimeout() {
  if (user_task_) {
    user_task_();
  }
}

}  // namespace base
