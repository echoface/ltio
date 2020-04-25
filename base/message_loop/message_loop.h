#ifndef _BASE_MSG_EVENT_LOOP_H_H
#define _BASE_MSG_EVENT_LOOP_H_H

#include <atomic>
#include <bits/stdint-uintn.h>
#include <functional>
#include <thread>
#include <queue>
#include <list>
#include <mutex>
#include <condition_variable>
#include <utility>

#include "base/closure/closure_task.h"
#include "base/closure/location.h"
#include "fd_event.h"
#include "event_pump.h"

#include "glog/logging.h"
#include "base/base_constants.h"
#include "base/queue/task_queue.h"
#include "base/memory/spin_lock.h"
#include "base/memory/scoped_ref_ptr.h"

namespace base {

enum LoopState {
  ST_INITING = 1,
  ST_STARTED = 2,
  ST_STOPED  = 3
};

class MessageLoop : public PumpDelegate {
  public:
    static MessageLoop* Current();

    typedef enum {
      TaskTypeDefault = 0,
      TaskTypeReply   = 1,
      TaskTypeCtrl    = 2
    } ScheduledTaskType;


    MessageLoop();
    virtual ~MessageLoop();

    bool PostTask(TaskBasePtr);

    bool PostDelayTask(TaskBasePtr, uint32_t milliseconds);

    template<class Functor>
    bool PostTask(const Location& location, const Functor& closure) {
      return PostTask(TaskBasePtr(location, closure));
    }

    template <typename Functor, typename... Args>
    bool PostTask(const Location& location, Functor&& functor, Args&&... args) {
      return PostTask(
        base::CreateClosure(location, std::bind(std::forward(functor), std::forward(args...))));
    }

    /* Task will run in target loop thread,
     * reply will run in Current() loop if it exist,
     * otherwise the reply task will run in target messageloop thread too*/
    template<class T, class Reply>
    bool PostTaskAndReply(const T& task,
                          const Reply& reply,
                          const Location& location) {
      return PostTaskAndReply(task, reply, this, location);
    }

    template<class T, class Reply>
    bool PostTaskAndReply(const T& task,
                          const Reply& reply,
                          MessageLoop* reply_loop,
                          const Location& location) {
      if (reply_loop == nullptr) {
        return PostTask(CreateTaskWithCallback(location, task, reply));
      }
      auto wrap_reply = [=]() ->void {
        reply_loop->PostTask(base::CreateClosure(location, reply));
      };
      return PostTask(CreateTaskWithCallback(location, task, wrap_reply));
    }

    void Start();
    bool IsInLoopThread() const;

    //t: millsecond for giveup cpu for waiting
    void WaitLoopEnd(int32_t t = 1);
    void SetLoopName(std::string name);
    void SetMinPumpTimeout(uint64_t ms);
    const std::string& LoopName() const {return loop_name_;}

    void QuitLoop();
    EventPump* Pump() {return &event_pump_;}

    void PumpStarted() override;
    void PumpStopped() override;
    uint64_t PumpTimeout() override;
  private:
    void ThreadMain();
    void SetThreadNativeName();

    bool PendingNestedTask(TaskBasePtr& task);

    void RunCommandTask(ScheduledTaskType t);

    // nested task: post another task in current loop
    // override from pump for nested task;
    void RunNestedTask() override;
    void RunTimerClosure(const TimerEventList&) override;

    int Notify(int fd, const void* data, size_t count);
  private:

    std::atomic_int status_;
    std::atomic_int has_join_;
    std::atomic_flag running_;

    std::mutex start_stop_lock_;
    std::condition_variable cv_;

    std::string loop_name_;
    std::unique_ptr<std::thread> thread_ptr_;

    int task_fd_ = -1;
    RefFdEvent task_event_;
    std::atomic_flag notify_flag_;
    ConcurrentTaskQueue scheduled_tasks_;
    std::list<TaskBasePtr> in_loop_tasks_;

    // pipe just use for loop control
    int wakeup_pipe_in_ = -1;
    int wakeup_pipe_out_ = -1;
    RefFdEvent wakeup_event_;

    EventPump event_pump_;
    uint64_t pump_timeout_ = 2000;
};

} //end namespace
#endif
