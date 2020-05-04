#ifndef _BASE_CORO_CONFIG_H_
#define _BASE_CORO_CONFIG_H_

enum class CoroState {
  kInit    = 0,
  kRunning = 1,
  kPaused  = 2,
  kDone    = 3
};

#if not defined(COROUTINE_STACK_SIZE)
#define COROUTINE_STACK_SIZE 262144 // 32768 * sizeof(void*)
#endif

#endif
