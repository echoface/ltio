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

#ifndef BASE_REPEATING_TIMER_H_H
#define BASE_REPEATING_TIMER_H_H

#include <inttypes.h>
#include <atomic>
#include <condition_variable>
#include <memory>
#include <mutex>

#include <base/lt_macro.h>
#include <base/closure/closure_task.h>

#include "message_loop.h"

namespace base {
// Sample RepeatingTimer usage:
//
//   class MyClass {
//    public:
//     void StartDoingStuff() {
//       timer_.Start(delay_ms, ClosureTask);
//     }
//     void StopStuff() {
//       timer_.Stop();
//     }
//    private:
//     void DoStuff() {
//       // This method is called every second to do stuff.
//       ...
//     }
//     base::RepeatingTimer timer_;
//   };
//
class MessageLoop;

/*start and stop are not thread safe*/
class RepeatingTimer {
public:
  // new a timer with spcial runner loop
  RepeatingTimer(MessageLoop* loop);
  ~RepeatingTimer();

  void Start(uint64_t ms, LtClosure user_task);
  // a sync stop
  void Stop();

  bool Running() const;

private:
  void OnTimeout();

  bool running_;
  LtClosure user_task_;
  MessageLoop* runner_loop_ = NULL;

  std::unique_ptr<TimeoutEvent> timeout_event_;

  std::mutex m;
  std::condition_variable cv;
  DISALLOW_COPY_AND_ASSIGN(RepeatingTimer);
};

}  // namespace base
#endif
