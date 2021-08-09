#ifndef __LT_CORO_MUTEX_H_H_
#define __LT_CORO_MUTEX_H_H_

#include <deque>
#include "base/lt_micro.h"
#include "base/closure/closure_task.h"
#include "base/memory/spin_lock.h"

namespace co {

/**
 * CoMutex is a mutex lock for coroutines
 * will abort in none-coro context runtime
 */
class CoMutex {
public:
  CoMutex();
  ~CoMutex();

  void lock();

  void unlock();

  bool try_lock();

private:
  CoMutex(CoMutex&& m) = delete;

  bool locked_;
  base::SpinLock lock_;
  std::deque<base::LtClosure> waiters_;
  DISALLOW_COPY_AND_ASSIGN(CoMutex);
};

}  // namespace base

#endif
