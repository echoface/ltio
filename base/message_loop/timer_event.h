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

#ifndef BASE_TIMER_EVENT_H_
#define BASE_TIMER_EVENT_H_

#include <cinttypes>
#include <base/time/timestamp.h>
#include <base/closure/closure_task.h>

namespace base {

typedef std::unique_ptr<TaskBase> UniqueTimerTask;

class TimerEvent {
public:
  typedef std::shared_ptr<TimerEvent> RefTimerEvent;
  static RefTimerEvent CreateOneShot(int64_t ms_later, UniqueTimerTask);
  static RefTimerEvent CreateRepeated(int64_t ms_later, UniqueTimerTask);

  TimerEvent(int64_t ms, bool once = true);
  TimerEvent(const Timestamp& t, bool once = true);
  ~TimerEvent();

  const Timestamp& Time() const;
  Timestamp& MutableTime() { return time_; }

  void Invoke();
  void SetTimerTask(UniqueTimerTask task);

  bool IsOneShotTimer() const {return once_;}
  int64_t Interval() const { return interval_;}
private:
  bool once_;

  Timestamp time_;
  int64_t interval_;

  UniqueTimerTask timer_task_;
};

typedef std::shared_ptr<TimerEvent> RefTimerEvent;

class TimerEventCompare {
public:
  bool operator()(const RefTimerEvent& lf, const RefTimerEvent& rt) {
    return lf->Time() > rt->Time();
  }
};

}//end base
#endif
