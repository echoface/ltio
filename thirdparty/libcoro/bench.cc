#include <stdio.h>
#include <atomic>
#include <time.h>
#include "stdlib.h"
#include <unistd.h>
#include "coro.h"
#include <iostream>

static std::atomic<long long int> counter = {0};

coro_context ctx, ctx2, mainctx;
struct coro_stack ctx_stack;
struct coro_stack ctx2_stack;

static void doo(void* ctx2) {
    while(1) {
      coro_transfer((coro_context*)ctx2, &ctx);
    }
}

static void foo(void* ctx) {
    while (counter.fetch_add(1) < 1000000000) {
      coro_transfer((coro_context*)ctx, &ctx2);
    }
    coro_transfer((coro_context*)ctx, &mainctx);
}

int main() {

    coro_stack_alloc(&ctx_stack, 8192);
    coro_stack_alloc(&ctx2_stack, 8192);
    std::cout << "real stack size:" << ctx2_stack.ssze << std::endl;

    coro_create(&mainctx, NULL, NULL, NULL, 0);
    coro_create(&ctx, foo, &ctx, ctx_stack.sptr, ctx_stack.ssze);
    coro_create(&ctx2, doo, &ctx2, ctx2_stack.sptr, ctx2_stack.ssze);

    struct timespec tstart={0,0}, tend={0,0};
    clock_gettime(CLOCK_MONOTONIC, &tstart);

    coro_transfer(&mainctx, &ctx);

    clock_gettime(CLOCK_MONOTONIC, &tend);
    printf("take about %.5f seconds\n",
           ((double)tend.tv_sec + 1.0e-9*tend.tv_nsec) -
           ((double)tstart.tv_sec + 1.0e-9*tstart.tv_nsec));
    coro_stack_free(&ctx_stack);
    coro_stack_free(&ctx2_stack);
    return 0;
}
