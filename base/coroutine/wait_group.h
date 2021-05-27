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

#ifndef _BASE_CORO_WAITGROUP_H_
#define _BASE_CORO_WAITGROUP_H_

#include <atomic>

#include <base/closure/closure_task.h>
#include <base/message_loop/message_loop.h>
#include <base/message_loop/timeout_event.h>

/*
 * usage: in coro context, WaitGroup can wait multi task done
 * without blocking backend pthread
 *
 * it's useful in case of broadcast or concurency task
 *
 * example code:
 * ```c++
 * auto wg = WaitGroup::New();
 * wg->Add(1);
 * //make sure copy wg into closre
 * CO_GO [wg, &]() {task1(); wg->Done();};
 *
 * wg.Add(1);
 * CO_GO [wg]() {task1(); wg->Done();};
 *
 * // max 1secon timeout waith;
 * // must in same context
 * wg->Wait(1000);
 * ```
 *
 * */

namespace base {

class WaitGroup : public EnableShared(WaitGroup) {
  public:
    enum Result {
      kSuccess = 0,
      kTimeout = 1,
    };

    static std::shared_ptr<WaitGroup> New();

    ~WaitGroup();

    void Add(int64_t count);

    void Done();

    Result Wait(int64_t timeout_ms = -1);
  private:
    WaitGroup();

    void OnTimeOut();
    void wakeup_internal();

    LtClosure resumer_;

    // only rw same loop
    Result result_status_;
    std::atomic_flag flag_;
    std::atomic<int64_t> wait_count_;

    MessageLoop* loop_ = nullptr;
    std::unique_ptr<TimeoutEvent> timeout_;
    DISALLOW_COPY_AND_ASSIGN(WaitGroup);
};

}

#endif
