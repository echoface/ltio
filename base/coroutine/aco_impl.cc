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
#include <thirdparty/libaco/aco.h>

namespace base {

typedef void (*CoroEntry)();

class Coroutine : public CoroBase, public EnableShared(Coroutine) {
public:
  friend class CoroRunner;

  static RefCoroutine CreateMain() { return RefCoroutine(new Coroutine()); }
  static RefCoroutine Create(CoroEntry entry, Coroutine* main_co) {
    return RefCoroutine(new Coroutine(entry, main_co));
  }

  ~Coroutine() {
    VLOG(GLOG_VTRACE) << "coroutine gone!"
                      << " count:" << SystemCoroutineCount()
                      << " state:" << StateToString(state_)
                      << " main:" << (is_main() ? "True" : "False");

    aco_destroy(coro_);
    coro_ = nullptr;

    if (sstk_) {
      CHECK(!IsRunning());
      coro_counter_.fetch_sub(1);
      aco_share_stack_destroy(sstk_);
      sstk_ = nullptr;
    }
  }

  /*NOTE: this will switch to main coro, be care for call this
   * must ensure call it when coro_fn finish, can't use return*/
  void Exit() {
    CHECK(!is_main());
    SetState(CoroState::kDone);
    aco_exit();
  }

  /*swich thread run context between main_coro and sub_coro*/
  void TransferTo(Coroutine* next) {
    CHECK(this != next);

    ResetElapsedTime();
    SetState(CoroState::kPaused);

    next->IncrResumeID();
    next->SetState(CoroState::kRunning);
    if (is_main()) {
      aco_resume(next->coro_);
    } else {
      aco_yield();
    }
  }

private:
  friend class CoroRunner;

  Coroutine() {
    aco_thread_init(NULL);
    coro_ = aco_create(NULL, NULL, 0, NULL, NULL);
  }

  Coroutine(CoroEntry entry, Coroutine* main_co) {
    SetState(CoroState::kInit);
    sstk_ = aco_share_stack_new(COROUTINE_STACK_SIZE);
    coro_ = aco_create(main_co->coro_, sstk_, 0, entry, this);

    coro_counter_.fetch_add(1);

    VLOG(GLOG_VTRACE) << "coroutine born!"
                      << " count:" << coro_counter_.load()
                      << " state:" << StateToString(state_)
                      << " main:" << (is_main() ? "True" : "False");
  }

  void SelfHolder(RefCoroutine& self) {
    CHECK(self.get() == this);
    self_holder_ = self;
  }

  void ReleaseSelfHolder() { self_holder_.reset(); };

  WeakCoroutine AsWeakPtr() { return shared_from_this(); }

  bool is_main() const { return sstk_ == nullptr; }

private:
  aco_t* coro_ = nullptr;
  aco_share_stack_t* sstk_ = nullptr;

  RefCoroutine self_holder_;
  DISALLOW_COPY_AND_ASSIGN(Coroutine);
};

}  // namespace base
