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

#ifndef _BASE_EVENTLOOP_TIMER_QUEUE_H_
#define _BASE_EVENTLOOP_TIMER_QUEUE_H_

#include <stack>
#include <vector>
#include "timer_event.h"

namespace base {

typedef struct {
  Timestamp time_;
  /*index in timer_events_*/
  uint32_t index_;
} TimerEntry;

class TimerEntryCompare {
public:
  bool operator()(const TimerEntry& lf, const TimerEntry& rt) {
    return lf.time_ > rt.time_;
  }
};

class TimerTaskQueue {
public:
  TimerTaskQueue(){};
  ~TimerTaskQueue();

  /*return the timer_index as identify for cancel*/
  uint32_t AddTimerEvent(RefTimerEvent& timer_event);

  /*return true if cancel success*/
  bool CancelTimerEvent(uint32_t timer_id);

  /* return the duration(millsecond) for next timerevent triggle*/
  uint64_t HandleExpiredTimer();
  void TestingPriorityQueue();
  uint64_t InexacTimeMillsecond() { return inexact_time_us_; }

private:
  void InvokeAndReScheduleIfNeed(uint32_t timerid);
  int64_t CalculateNextTimerDuration();

  typedef std::
      priority_queue<TimerEntry, std::vector<TimerEntry>, TimerEntryCompare>
          TimerIndexHeap;

  TimerIndexHeap timer_heap_;
  std::vector<RefTimerEvent> timer_events_;
  std::stack<uint32_t> reusable_timerid_;
  uint64_t inexact_time_us_ = 0;
};

}  // namespace base
#endif
