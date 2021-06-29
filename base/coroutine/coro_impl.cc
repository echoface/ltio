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

#include "coroutine.h"

#include <atomic>
#include <cinttypes>
#include <functional>
#include <memory>

#include <base/base_constants.h>
#include <base/base_micro.h>
#include <base/closure/closure_task.h>
#include <base/memory/spin_lock.h>
#include <thirdparty/libcoro/coro.h>

namespace {
base::SpinLock g_coro_lock;
}

namespace base {

typedef void (*CoroEntry)(void* arg);

class Coroutine : public coro_context,
                  public CoroBase,
                  public EnableShared(Coroutine) {
public:
  static RefCoroutine CreateMain() { return RefCoroutine(new Coroutine()); }
  static RefCoroutine Create(CoroEntry entry, Coroutine* main_co) {
    return RefCoroutine(new Coroutine(entry, main_co));
  }

  ~Coroutine() {
    VLOG(GLOG_VTRACE) << "coroutine gone!"
                      << " count:" << SystemCoroutineCount()
                      << " state:" << StateToString(state_)
                      << " main:" << (is_main() ? "True" : "False");
    // none main coroutine
    if (stack_.ssze != 0) {
      coro_stack_free(&stack_);
      coro_counter_.fetch_sub(1);
      CHECK(!IsRunning());
    }
  }

  void TransferTo(Coroutine* next) {
    CHECK(this != next);
    if (IsRunning()) {
      SetState(CoroState::kPaused);
    }
    next->IncrResumeID();
    next->SetState(CoroState::kRunning);

    ResetElapsedTime();
    coro_transfer(this, next);
  }

  void SelfHolder(RefCoroutine& self) {
    CHECK(self.get() == this);
    self_holder_ = self;
  }

private:
  friend class CoroRunner;

  // main coro
  Coroutine() {
    stack_.ssze = 0;
    stack_.sptr = nullptr;
    {
      base::SpinLockGuard guard(g_coro_lock);
      coro_create(this, NULL, NULL, NULL, 0);
    }
  }

  Coroutine(CoroEntry entry, Coroutine* main_co) {
    SetState(CoroState::kInit);

    stack_.ssze = 0;
    stack_.sptr = nullptr;
    int r = coro_stack_alloc(&stack_, COROUTINE_STACK_SIZE);
    LOG_IF(ERROR, r == 0) << "Failed Allocate Coroutine Stack";
    CHECK(r == 1);
    memset((coro_context*)this, 0, sizeof(coro_context));
    {
      base::SpinLockGuard guard(g_coro_lock);
      coro_create(this, entry, this, stack_.sptr, stack_.ssze);
    }
    coro_counter_.fetch_add(1);

    VLOG(GLOG_VTRACE) << "coroutine born!"
                      << " count:" << SystemCoroutineCount()
                      << " state:" << StateToString(state_)
                      << " main:" << (is_main() ? "True" : "False");
  }

  void ReleaseSelfHolder() { self_holder_.reset(); };

  WeakCoroutine AsWeakPtr() { return shared_from_this(); }

  bool is_main() const { return stack_.ssze == 0; }

private:
  coro_stack stack_;

  RefCoroutine self_holder_;
  DISALLOW_COPY_AND_ASSIGN(Coroutine);
};

}  // namespace base
