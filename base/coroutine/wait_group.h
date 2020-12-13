#ifndef _BASE_CORO_WAITGROUP_H_
#define _BASE_CORO_WAITGROUP_H_

#include <atomic>
#include <base/closure/closure_task.h>
#include <base/message_loop/timeout_event.h>

/*
 * usage: in coro context, WaitGroup can wait multi task done
 * without blocking backend pthread
 *
 * it's useful in case of broadcast or concurency task
 *
 * example code:
 * ```c++
 * WaitGroup wg;
 * wg.Add(1);
 * CO_GO [&]() {task1(); wg.Done();};
 *
 * wg.Add(1);
 * CO_GO []() {task1(); wg.Done();};
 *
 * wg.Wait(); //must in same context
 * ```
 * or
 * ```c++
 * WaitGroup wg;
 * wg.Add(1);
 * loop->PostTask(NewClosureWithCallback(task, std::bind(WaitGroup::Done, &wg)));
 * wg.Wait(1000); // max wait timeout 1000ms
 *
 * */

namespace base {

class WaitGroup {
  public:
    enum WaitResult {
      kSuccess = 0,
      kTimeout = 1,
    };
    WaitGroup();
    ~WaitGroup();

    void Add(int64_t count);

    void Done();

    WaitResult Wait(int64_t timeout_ms = -1);
  private:
    // WaitGroup only can create on stack
		void operator delete(void*) {}
    void* operator new(size_t) noexcept {return nullptr;}

    void OnTimeOut();

    LtClosure resumer_;

    std::atomic_flag flag_;
    std::atomic<int64_t> wait_count_;

    std::unique_ptr<TimeoutEvent> timeout_;
    DISALLOW_COPY_AND_ASSIGN(WaitGroup);
};

}

#endif
