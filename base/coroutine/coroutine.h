#ifndef COROUTINE_H_H_
#define COROUTINE_H_H_

#include <memory>
#include <functional>

#include <base/base_micro.h>


#if not defined(COROUTINE_STACK_SIZE)
#define COROUTINE_STACK_SIZE 262144 // 32768 * sizeof(void*)
#endif

namespace base {

enum class CoroState {
  kInit    = 0,
  kRunning = 1,
  kPaused  = 2,
  kDone    = 3
};

int64_t SystemCoroutineCount();
std::string StateToString(CoroState st);

class Coroutine;
typedef std::weak_ptr<Coroutine> WeakCoroutine;
typedef std::shared_ptr<Coroutine> RefCoroutine;

} //end if base
#endif
