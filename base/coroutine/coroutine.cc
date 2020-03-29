
#include "coroutine.h"

#include <vector>
#include <atomic>
#include <mutex>
#include "glog/logging.h"
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
RefCoroutine Coroutine::Create(CoroEntry entry, bool main) {
  return RefCoroutine(new Coroutine(entry, main));
}

Coroutine::Coroutine(CoroEntry entry, bool main) :
  identify_(0) {

  stack_.ssze = 0;
  stack_.sptr = nullptr;

  state_ = CoroState::kInitialized;
  if (main) {
    {
      base::SpinLockGuard guard(g_coro_lock);
      coro_create(this, NULL, NULL, NULL, 0);
    }
    state_ = CoroState::kRunning;
    return;
  }

  int r = coro_stack_alloc(&stack_, COROUTINE_STACK_SIZE);
  LOG_IF(ERROR, r == 0) << "Failed Allocate Coroutine Stack";
  CHECK(r == 1);

  memset((coro_context*)this, 0, sizeof(coro_context));
  {
    base::SpinLockGuard guard(g_coro_lock);
    coro_create(this, entry, this, stack_.sptr, stack_.ssze);
  }
  g_counter.fetch_add(1);
  VLOG(GLOG_VTRACE) << "coroutine born! count:" << g_counter.load()
    << " st:" << StateToString()
    << " main:" << (is_main() ? "True" : "False");
}

Coroutine::~Coroutine() {
  if (stack_.ssze != 0) { //none main coroutine
    coro_stack_free(&stack_);
    g_counter.fetch_sub(1);
    CHECK(state_ != CoroState::kRunning);
  }

  VLOG(GLOG_VTRACE) << "coroutine gone! count:" << g_counter.load()
    << " st:" << StateToString()
    << " main:" << (is_main() ? "True" : "False");
}

void Coroutine::SelfHolder(RefCoroutine& self) {
  CHECK(self.get() == this);
  self_holder_ = self;
}

void Coroutine::Reset() {
  state_ = CoroState::kInitialized;
}

std::string Coroutine::StateToString() const {
  switch(state_) {
    case CoroState::kDone:
      return "Done";
    case CoroState::kPaused:
      return "Paused";
    case CoroState::kRunning:
      return "Running";
    case CoroState::kInitialized:
      return "Initialized";
    default:
      break;
  }
  return "Unknown";
}

}//end base
