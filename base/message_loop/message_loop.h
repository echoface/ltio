#ifndef _BASE_MSG_EVENT_LOOP_H_H
#define _BASE_MSG_EVENT_LOOP_H_H

#include <atomic>
#include <condition_variable>
#include <functional>
#include <list>
#include <mutex>
#include <queue>
#include <thread>
#include <utility>

#include "glog/logging.h"

#include "base/base_constants.h"
#include "base/closure/closure_task.h"
#include "base/closure/location.h"
#include "base/memory/scoped_ref_ptr.h"
#include "base/memory/spin_lock.h"
#include "base/queue/task_queue.h"
#include "event_pump.h"
#include "fd_event.h"

namespace base {

enum LoopState {
  ST_INITING = 1,
  ST_STARTED = 2,
  ST_STOPED  = 3
};

class MessageLoop;
class PersistRunner {
  public:
    virtual void Sched() = 0;
    virtual void LoopGone(MessageLoop* loop) {};
};


class MessageLoop : public PumpDelegate,
                    public FdEvent::Handler {
  public:
    static MessageLoop* Current();

    typedef enum {
      TaskTypeDefault = 0,
      TaskTypeReply   = 1,
      TaskTypeCtrl    = 2
    } ScheduledTaskType;


    MessageLoop();
    virtual ~MessageLoop();

    bool PostTask(TaskBasePtr&&);

    bool PostDelayTask(TaskBasePtr, uint32_t milliseconds);

    template<class Functor>
    bool PostTask(const Location& location, const Functor& closure) {
      return PostTask(CreateClosure(location, closure));
    }

    template <typename Functor, typename... Args>
    bool PostTask(const Location& location, Functor&& functor, Args&&... args) {
      return PostTask(CreateClosure(location, std::bind(functor, args...)));
    }

    /* Task will run in target loop thread,
     * reply will run in Current() loop if it exist,
     * otherwise the reply task run in target loop thread too*/
    template<class T, class R>
    bool PostTaskAndReply(const Location& location,
                          const T& task,
                          const R& reply) {
      return PostTaskAndReply(location, task, reply, this);
    }

    template<class T, class R>
    bool PostTaskAndReply(const Location& location,
                          const T& task,
                          const R& reply,
                          MessageLoop* reply_loop) {
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

    void WakeUpIfNeeded();
    void InstallPersistRunner(PersistRunner* runner);
    //t: millsecond for giveup cpu for waiting
    void WaitLoopEnd(int32_t t = 1);
    void SetLoopName(std::string name);
    const std::string& LoopName() const {return loop_name_;}

    void QuitLoop();
    EventPump* Pump() {return &event_pump_;}

    void PumpStarted() override;
    void PumpStopped() override;
    uint64_t PumpTimeout() override;
  private:
    void ThreadMain();
    void SetThreadNativeName();

    bool PendingNestedTask(TaskBasePtr&& task);

    void RunCommandTask(ScheduledTaskType t);

    // nested task: post another task in current loop
    // override from pump for nested task;
    void RunNestedTask() override;
    void RunScheduledTask();
    void RunTimerClosure(const TimerEventList&) override;

    int Notify(int fd, const void* data, size_t count);

    bool HandleRead(FdEvent* fd_event) override;
    bool HandleWrite(FdEvent* fd_event) override;
    bool HandleError(FdEvent* fd_event) override;
    bool HandleClose(FdEvent* fd_event) override;
  private:

    std::atomic<int> status_;
    std::atomic<int> has_join_;
    std::atomic_flag running_;

    std::mutex start_stop_lock_;
    std::condition_variable cv_;

    std::string loop_name_;
    std::unique_ptr<std::thread> thread_ptr_;

    RefFdEvent task_event_;
    std::atomic_flag notify_flag_;

    ConcurrentTaskQueue scheduled_tasks_;
    std::vector<TaskBasePtr> in_loop_tasks_;

    std::list<PersistRunner*> persist_runner_;

    // pipe just use for loop control
    int wakeup_pipe_in_ = -1;
    RefFdEvent wakeup_event_;

    EventPump event_pump_;
    DISALLOW_COPY_AND_ASSIGN(MessageLoop);
};

} //end namespace
#endif
