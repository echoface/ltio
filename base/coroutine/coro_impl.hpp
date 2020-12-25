#include "coroutine.h"

#include <atomic>
#include <cinttypes>
#include <functional>
#include <memory>

#include <base/base_micro.h>
#include <base/base_constants.h>
#include <base/closure/closure_task.h>
#include <base/memory/spin_lock.h>
#include <thirdparty/libcoro/coro.h>

namespace {

base::SpinLock g_coro_lock;
std::atomic<int64_t> g_counter = {0};
}

namespace base {

int64_t SystemCoroutineCount() {
  return g_counter.load();
}

typedef void (*LibCoroEntry)(void* arg);

class Coroutine : public coro_context,
                  public EnableShared(Coroutine) {
public:
  static RefCoroutine CreateMain() {
    return RefCoroutine(new Coroutine());
  }
  static RefCoroutine Create(LibCoroEntry entry) {
    return RefCoroutine(new Coroutine(entry));
  }

  ~Coroutine() {
    VLOG(GLOG_VTRACE) << "coroutine gone!"
      << " count:" << g_counter.load()
      << " state:" << StateToString(state_)
      << " main:" << (is_main() ? "True" : "False");
    //none main coroutine
    if (stack_.ssze != 0) {
      coro_stack_free(&stack_);
      g_counter.fetch_sub(1);
      CHECK(!IsRunning());
    }
  }

  CoroState Status() const {return state_;}

  uint64_t ResumeId() const {return resume_cnt_;}

  inline bool IsPaused() const {return state_ == CoroState::kPaused;}

  inline bool IsRunning() const {return state_ == CoroState::kRunning;}

  bool CanResume(int64_t resume_id) {
    return IsPaused() && resume_cnt_ == resume_id;
  }

  void TransferTo(Coroutine* next) {
    CHECK(this != next);
    if (state_ == CoroState::kRunning) {
      SetCoroState(CoroState::kPaused);
    }
    next->resume_cnt_++;
    next->SetCoroState(CoroState::kRunning);

    coro_transfer(this, next);
  }

  void SelfHolder(RefCoroutine& self) {
    CHECK(self.get() == this);
    self_holder_ = self;
  }

  void SetCoroState(CoroState st) {state_ = st;}

  void ReleaseSelfHolder() {self_holder_.reset();};

  WeakCoroutine AsWeakPtr() {return shared_from_this();}

private:
  friend class CoroRunner;

  //main coro
  Coroutine()
    : resume_cnt_(0) {
    stack_.ssze = 0;
    stack_.sptr = nullptr;
    state_ = CoroState::kRunning;
    {
      base::SpinLockGuard guard(g_coro_lock);
      coro_create(this, NULL, NULL, NULL, 0);
    }
  }

  Coroutine(LibCoroEntry entry)
    : resume_cnt_(0) {

      stack_.ssze = 0;
      stack_.sptr = nullptr;
      state_ = CoroState::kInit;

      int r = coro_stack_alloc(&stack_, COROUTINE_STACK_SIZE);
      LOG_IF(ERROR, r == 0) << "Failed Allocate Coroutine Stack";
      CHECK(r == 1);

      memset((coro_context*)this, 0, sizeof(coro_context));
      {
        base::SpinLockGuard guard(g_coro_lock);
        coro_create(this, entry, this, stack_.sptr, stack_.ssze);
      }
      g_counter.fetch_add(1);
      VLOG(GLOG_VTRACE) << "coroutine born!"
        << " count:" << g_counter.load()
        << " state:" << StateToString(state_)
        << " main:" << (is_main() ? "True" : "False");
  }

  bool is_main() const {return stack_.ssze == 0;}

private:
  CoroState state_;
  coro_stack stack_;

  /* resume_cnt_ here use to resume correctly when
   * yielded context be resumed multi times */
  uint64_t resume_cnt_;

  RefCoroutine self_holder_;
  DISALLOW_COPY_AND_ASSIGN(Coroutine);
};

}//end base
