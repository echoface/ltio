#ifndef _BASE_CORO_WAITGROUP_H_
#define _BASE_CORO_WAITGROUP_H_

#include <atomic>
#include <base/closure/closure_task.h>
#include <base/message_loop/timeout_event.h>

namespace base {

class WaitGroup {
  public:
    WaitGroup();
    ~WaitGroup();

    void Add(int64_t count);

    void Done();
    void Wait(int64_t timeout_ms = -1);
  private:
    void wake_up();
    base::StlClosure resumer_;

    TimeoutEvent* timeout_;
    std::atomic_flag flag_;
    std::atomic_flag wakeup_flag_;
    std::atomic_int64_t wait_count_;
    DISALLOW_COPY_AND_ASSIGN(WaitGroup);
};

}

#endif
