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

#ifndef COROUTINE_H_H_
#define COROUTINE_H_H_

#include <atomic>
#include <functional>
#include <memory>

#include <base/base_micro.h>
#include "base/time/time_utils.h"

#if not defined(COROUTINE_STACK_SIZE)
#define COROUTINE_STACK_SIZE 262144  // 32768 * sizeof(void*)
#endif

namespace base {

enum class CoroState { kInit = 0, kRunning = 1, kPaused = 2, kDone = 3 };

std::string StateToString(CoroState st);
class CoroBase {
public:
  static size_t SystemCoroutineCount();

protected:
  friend class CoroRunner;

  CoroBase() : state_(CoroState::kInit), resume_cnt_(0) {}

  void ResetElapsedTime() { timestamp_us_ = base::time_us(); }
  uint64_t ElapsedTime() const { return time_us() - timestamp_us_; }

  inline void IncrResumeID() { resume_cnt_++; }
  inline bool CanResume(int64_t resume_id) {
    return IsPaused() && resume_cnt_ == resume_id;
  }
  inline uint64_t ResumeID() const { return resume_cnt_; }

  inline CoroState Status() const { return state_; }
  inline void SetState(CoroState st) { state_ = st; }
  inline bool IsPaused() const { return state_ == CoroState::kPaused; }
  inline bool IsRunning() const { return state_ == CoroState::kRunning; }

  CoroState state_;

  /* resume_cnt_ here use to resume correctly when
   * yielded context be resumed multi times */
  uint64_t resume_cnt_;

  /* a timestamp record for every coro task 'continuous'
   * time consume, it will reset every yield action*/
  int64_t timestamp_us_;

  /* a global coro counter*/
  static std::atomic<int64_t> coro_counter_;
};

class Coroutine;
typedef std::weak_ptr<Coroutine> WeakCoroutine;
typedef std::shared_ptr<Coroutine> RefCoroutine;

}  // namespace base
#endif
