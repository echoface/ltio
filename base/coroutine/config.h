#ifndef _BASE_CORO_CONFIG_H_
#define _BASE_CORO_CONFIG_H_

//#define COROUTINE_STACK_SIZE 16384 // 32768 * sizeof(void*)
#if not defined(COROUTINE_STACK_SIZE)
#define COROUTINE_STACK_SIZE 262144 // 32768 * sizeof(void*)
#endif

#endif
