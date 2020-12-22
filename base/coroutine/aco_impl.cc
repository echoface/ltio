#include "aco_impl.h"

#include <atomic>
#include <mutex>
#include <vector>

#include "glog/logging.h"

#include "config.h"
#include <base/base_constants.h>
#include <base/memory/spin_lock.h>

namespace base {

static base::SpinLock g_coro_lock;
static std::atomic_int64_t g_counter;

//static
int64_t SystemCoroutineCount() {
  return g_counter.load();
}

//static
RefCoroutine Coroutine::Create(CoroEntry entry, Coroutine* main_co) {
  return RefCoroutine(new Coroutine(entry, main_co));
}

RefCoroutine Coroutine::CreateMain() {
  return RefCoroutine(new Coroutine());
}

Coroutine::Coroutine()
  : resume_cnt_(0) {

    aco_thread_init(NULL);
    state_ = CoroState::kRunning;
    coro_ = aco_create(NULL, NULL, 0, NULL, NULL);
}

Coroutine::Coroutine(CoroEntry entry, Coroutine* main_co)
  : resume_cnt_(0) {

  state_ = CoroState::kInit;
  sstk_ = aco_share_stack_new(COROUTINE_STACK_SIZE);
  coro_ = aco_create(main_co->coro_, sstk_, 0, entry, this);

  g_counter.fetch_add(1);

  VLOG(GLOG_VTRACE) << "coroutine born! count:" << g_counter.load()
    << " st:" << StateToString()
    << " main:" << (is_main() ? "True" : "False");
}

Coroutine::~Coroutine() {

  aco_destroy(coro_);

  if (sstk_) {
    CHECK(!IsRunning());

    g_counter.fetch_sub(1);
    aco_share_stack_destroy(sstk_);
    sstk_ = nullptr;
  }

  VLOG(GLOG_VTRACE) << "coroutine gone! count:" << g_counter.load()
    << " st:" << StateToString()
    << " main:" << (is_main() ? "True" : "False");
}

void Coroutine::SelfHolder(RefCoroutine& self) {
  CHECK(self.get() == this);
  self_holder_ = self;
}

std::string Coroutine::StateToString() const {
  switch(state_) {
    case CoroState::kDone:
      return "Done";
    case CoroState::kPaused:
      return "Paused";
    case CoroState::kRunning:
      return "Running";
    case CoroState::kInit:
      return "Initialized";
    default:
      break;
  }
  return "Unknown";
}

bool Coroutine::CanResume(int64_t resume_id) {
  return IsPaused() && resume_cnt_ == resume_id;
}

void Coroutine::Exit() {
  CHECK(!is_main());
  SetCoroState(CoroState::kDone);
  aco_exit();
}

void Coroutine::TransferTo(Coroutine* next) {
  CHECK(this != next);

  this->SetCoroState(CoroState::kPaused);

  next->resume_cnt_++;
  next->SetCoroState(CoroState::kRunning);
  if (!is_main()) {
    aco_yield();
  } else {
    aco_resume(next->coro_);
  }
}

}//end base
