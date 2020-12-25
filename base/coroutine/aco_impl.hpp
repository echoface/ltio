#include "coroutine.h"

#include <atomic>
#include <cinttypes>
#include <functional>
#include <memory>

#include <base/base_micro.h>
#include <base/base_constants.h>
#include <base/closure/closure_task.h>
#include <thirdparty/libaco/aco.h>

namespace {
  std::atomic<int64_t> g_counter = {0};
}

namespace base {

int64_t SystemCoroutineCount() {
  return g_counter.load();
}

typedef void (*CoroEntry)();

class Coroutine :public EnableShared(Coroutine) {
public:
  friend class CoroRunner;

  static RefCoroutine CreateMain() {
    return RefCoroutine(new Coroutine());
  }
  static RefCoroutine Create(CoroEntry entry, Coroutine* main_co) {
    return RefCoroutine(new Coroutine(entry, main_co));
  }

  ~Coroutine() {

    VLOG(GLOG_VTRACE) << "coroutine gone!"
      << " count:" << g_counter.load()
      << " state:" << StateToString(state_)
      << " main:" << (is_main() ? "True" : "False");

    aco_destroy(coro_);
    coro_ = nullptr;

    if (sstk_) {
      CHECK(!IsRunning());
      g_counter.fetch_sub(1);
      aco_share_stack_destroy(sstk_);
      sstk_ = nullptr;
    }
  }

  CoroState Status() const {return state_;}

  uint64_t ResumeId() const {return resume_cnt_;}

  inline bool IsPaused() const {return state_ == CoroState::kPaused;}

  inline bool IsRunning() const {return state_ == CoroState::kRunning;}

  /*NOTE: this will switch to main coro, be care for call this
   * must ensure call it when coro_fn finish, can't use return*/
  void Exit() {
    CHECK(!is_main());
    SetCoroState(CoroState::kDone);
    aco_exit();
  }

  /*swich thread run context between main_coro and sub_coro*/
  void TransferTo(Coroutine* next) {
    CHECK(this != next);

    SetCoroState(CoroState::kPaused);
    next->resume_cnt_++;
    next->SetCoroState(CoroState::kRunning);
    if (is_main()) {
      aco_resume(next->coro_);
    } else {
      aco_yield();
    }
  }

  bool CanResume(int64_t resume_id) {
    return IsPaused() && resume_cnt_ == resume_id;
  }

private:
  Coroutine()
    : resume_cnt_(0) {
    aco_thread_init(NULL);
    state_ = CoroState::kRunning;
    coro_ = aco_create(NULL, NULL, 0, NULL, NULL);
  }

  Coroutine(CoroEntry entry, Coroutine* main_co)
    : resume_cnt_(0) {

    state_ = CoroState::kInit;
    sstk_ = aco_share_stack_new(COROUTINE_STACK_SIZE);
    coro_ = aco_create(main_co->coro_, sstk_, 0, entry, this);

    g_counter.fetch_add(1);

    VLOG(GLOG_VTRACE) << "coroutine born!"
      << " count:" << g_counter.load()
      << " state:" << StateToString(state_)
      << " main:" << (is_main() ? "True" : "False");
  }

  void SelfHolder(RefCoroutine& self) {
    CHECK(self.get() == this);
    self_holder_ = self;
  }

  void SetCoroState(CoroState st) {state_ = st;}

  void ReleaseSelfHolder() {self_holder_.reset();};

  WeakCoroutine AsWeakPtr() {return shared_from_this();}

  bool is_main() const {return sstk_ == nullptr;}
private:
  CoroState state_;
  uint64_t resume_cnt_;

  aco_t* coro_ = nullptr;
  aco_share_stack_t* sstk_ = nullptr;

  RefCoroutine self_holder_;
  DISALLOW_COPY_AND_ASSIGN(Coroutine);
};

}//end base

