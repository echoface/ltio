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


#include "timer_event.h"
#include "glog/logging.h"

namespace base {

  //static
RefTimerEvent TimerEvent::CreateOneShot(int64_t ms_later, UniqueTimerTask task) {
  RefTimerEvent t(std::make_shared<TimerEvent>(ms_later));
  t->SetTimerTask(std::move(task));
  return std::move(t);
}

RefTimerEvent TimerEvent::CreateRepeated(int64_t ms_later, UniqueTimerTask task) {
  RefTimerEvent t(std::make_shared<TimerEvent>(ms_later, false));
  t->SetTimerTask(std::move(task));
  return std::move(t);
}

TimerEvent::TimerEvent(const Timestamp& t, bool once)
  : once_(once),
    time_(t),
    interval_(0) {
  Timestamp now = Timestamp::Now();

  interval_ = t.AsMillsecond() - now.AsMillsecond();

  LOG_IF(INFO, time_ < now) << " Init A Invalid (out date) TimerEvent";

  if (interval_ < 0) {
    interval_ = (-interval_);
  }
}

TimerEvent::TimerEvent(int64_t ms, bool once)
  : once_(once),
    time_(Timestamp::NMillisecondLater(ms)),
    interval_(ms) {
}

TimerEvent::~TimerEvent() {
  //LOG(INFO) << "TimerEvent Gone";
}

const Timestamp& TimerEvent::Time() const {
  return time_;
}

void TimerEvent::SetTimerTask(UniqueTimerTask task) {
  timer_task_ = std::move(task);
}

void TimerEvent::Invoke() {
  if (timer_task_) {
    timer_task_->Run();
  }
}

}
