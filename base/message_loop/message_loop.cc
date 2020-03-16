#include "message_loop.h"

#include <sys/types.h>
#include <sys/time.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>

#include <csignal>
#include <limits.h>
#include <string>
#include <sstream>
#include <functional>
#include <chrono>
#include <map>

#include <sys/eventfd.h>

#include <iostream>
#include "file_util_linux.h"
#include "timeout_event.h"
#include <base/time/time_utils.h>
#include <base/utils/sys_error.h>

namespace base {

static const char kQuit = 1;
static const int64_t kTaskFdCounter = 1;

//static thread safe
static thread_local MessageLoop* threadlocal_current_ = NULL;
MessageLoop* MessageLoop::Current() {
  return threadlocal_current_;
}

class TimeoutTaskHelper : public TaskBase {
public:
  TimeoutTaskHelper(TaskBasePtr task, EventPump* pump, uint64_t ms)
    : TaskBase(task->TaskLocation()),
      timeout_fn_(std::move(task)),
      event_pump_(pump),
      delay_ms_(ms),
      schedule_time_(time_ms()) {}

  void Run() override {

    uint64_t has_passed_time = time_ms() - schedule_time_;
    int64_t new_delay_ms = delay_ms_ - has_passed_time;
    if (new_delay_ms <= 0) {
      return timeout_fn_->Run();
    }

    VLOG(GLOG_VINFO) <<  "Re-Schedule timer " << new_delay_ms << " ms";
    TimeoutEvent* timeout_ev = TimeoutEvent::CreateOneShot(new_delay_ms, true);
    timeout_ev->InstallTimerHandler(std::move(timeout_fn_));
    event_pump_->AddTimeoutEvent(timeout_ev);
  }
private:
  TaskBasePtr timeout_fn_;
  EventPump* event_pump_;
  const uint64_t delay_ms_;
  const uint64_t schedule_time_;
};

MessageLoop::MessageLoop()
  : running_(ATOMIC_FLAG_INIT),
    wakeup_pipe_in_(0),
    wakeup_pipe_out_(0),
    event_pump_(this) {

  notify_flag_.clear();
  status_.store(ST_INITING);
  has_join_.store(0);

  int fds[2];
  bool ret = CreateLocalNoneBlockingPipe(fds);
  CHECK(ret);

  wakeup_pipe_out_ = fds[0];
  wakeup_pipe_in_ = fds[1];

  wakeup_event_ = FdEvent::Create(wakeup_pipe_out_, LtEv::LT_EVENT_READ);
  wakeup_event_->SetReadCallback(std::bind(&MessageLoop::RunCommandTask, this, ScheduledTaskType::TaskTypeCtrl));

  task_fd_ = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
  task_event_ = FdEvent::Create(task_fd_, LtEv::LT_EVENT_READ);
  task_event_->SetReadCallback(std::bind(&MessageLoop::RunCommandTask, this, ScheduledTaskType::TaskTypeDefault));

  running_.clear();
}

MessageLoop::~MessageLoop() {
  LOG(INFO) << "MessageLoop: [" << LoopName() << "] Gone...";
  CHECK(status_.load() != ST_STARTED || !IsInLoopThread());

  QuitLoop();
  Notify(wakeup_pipe_in_, &kQuit, sizeof(kQuit));
  WaitLoopEnd();
  wakeup_event_.reset();
  close(wakeup_pipe_in_);

  task_event_.reset();
  wakeup_pipe_in_ = -1;
  wakeup_pipe_out_ = -1;
}

void MessageLoop::SetLoopName(std::string name) {
  loop_name_ = name;
}

bool MessageLoop::IsInLoopThread() const {
  return event_pump_.IsInLoopThread();
}

void MessageLoop::Start() {
  if (running_.test_and_set(std::memory_order_acquire)) {
    LOG(INFO) << "Messageloop [" << LoopName() << "] aready runing...";
    return;
  }

  thread_ptr_.reset(new std::thread(std::bind(&MessageLoop::ThreadMain, this)));
  {
    std::unique_lock<std::mutex> lk(start_stop_lock_);
    while(cv_.wait_for(lk, std::chrono::milliseconds(1)) == std::cv_status::timeout) {
      if (status_.load() == ST_STARTED) break;
    }
  }
}

void MessageLoop::WaitLoopEnd(int32_t ms) {
  //can't wait stop in loop thread
  CHECK(!IsInLoopThread());
  //first join, others wait util loop end
  int32_t has_join = has_join_.exchange(1);
  if (has_join == 0 && thread_ptr_ && thread_ptr_->joinable()) {
    LOG(INFO) << " join here wait loop end";
    thread_ptr_->join();
  } else {
    LOG(INFO) << " cv wait here wait loop end";
    std::unique_lock<std::mutex> lk(start_stop_lock_);
    while(cv_.wait_for(lk, std::chrono::milliseconds(100)) == std::cv_status::timeout) {
      if (status_.load() != ST_STARTED) break;
    }
  }
}

void MessageLoop::PumpStarted() {
  status_.store(ST_STARTED);
  cv_.notify_all();
}

void MessageLoop::PumpStopped() {
  status_.store(ST_STOPED);
  cv_.notify_all();
}

void MessageLoop::ThreadMain() {
  threadlocal_current_ = this;
  SetThreadNativeName();
  event_pump_.SetLoopThreadId(std::this_thread::get_id());

  event_pump_.InstallFdEvent(task_event_.get());
  event_pump_.InstallFdEvent(wakeup_event_.get());

  LOG(INFO) << "MessageLoop: [" << loop_name_ << "] Start Running";

  //delegate_->BeforeLoopRun();
  event_pump_.Run();
  //delegate_->AfterLoopRun();

  event_pump_.RemoveFdEvent(wakeup_event_.get());
  event_pump_.RemoveFdEvent(task_event_.get());

  threadlocal_current_ = NULL;
  LOG(INFO) << "MessageLoop: [" << loop_name_ << "] Stop Running";

}

bool MessageLoop::PostDelayTask(TaskBasePtr task, uint32_t ms) {
  LOG_IF(ERROR, status_ != ST_STARTED) << "loop not started, loc:" << task->TaskLocation().ToString();

  if (!IsInLoopThread()) {
    return PostTask(TaskBasePtr(new TimeoutTaskHelper(std::move(task), &event_pump_, ms)));
  }

  TimeoutEvent* timeout_ev = TimeoutEvent::CreateOneShot(ms, true);
  timeout_ev->InstallTimerHandler(std::move(task));
  event_pump_.AddTimeoutEvent(timeout_ev);
  return true;
}

bool MessageLoop::PendingNestedTask(TaskBasePtr& task) {
  DCHECK(IsInLoopThread());

  in_loop_tasks_.push_back(std::move(task));
  //if (!notify_flag_.test_and_set()) {
  Notify(task_fd_, &kTaskFdCounter, sizeof(kTaskFdCounter));
  //}
  return true;
}

bool MessageLoop::PostTask(TaskBasePtr task) {
  LOG_IF(ERROR, status_ != ST_STARTED) << "loop not started, loc:" << task->TaskLocation().ToString();

  if (IsInLoopThread()) {
    return PendingNestedTask(task);
  }

  bool ret = scheduled_tasks_.enqueue(std::move(task));
  //if (!notify_flag_.test_and_set()) {
  Notify(task_fd_, &kTaskFdCounter, sizeof(kTaskFdCounter));
  //}
  return ret;
}

void MessageLoop::RunTimerClosure(const TimerEventList& timer_evs) {
  for (auto& timeout_event : timer_evs) {
    timeout_event->Invoke();
  }
}

void MessageLoop::RunNestedTask() {
  DCHECK(IsInLoopThread());
  for (auto& task : in_loop_tasks_) {
    task->Run();
  }
  in_loop_tasks_.clear();
}

void MessageLoop::RunCommandTask(ScheduledTaskType type) {
  switch(type) {
    case ScheduledTaskType::TaskTypeDefault: {
      notify_flag_.clear();
      uint64_t count = 0;
      CHECK(sizeof(count) == ::read(task_fd_, &count, sizeof(count)));

      TaskBasePtr task;  // instead of pinco *p;
      while (scheduled_tasks_.try_dequeue(task)) {
        DCHECK(task);
        task->Run();
      }
    } break;
    case ScheduledTaskType::TaskTypeCtrl: {
      DCHECK(wakeup_event_->fd() == wakeup_pipe_out_);
      char buf;
      CHECK(sizeof(buf) == ::read(wakeup_pipe_out_, &buf, sizeof(buf)));
      switch (buf) {
        case kQuit: {
          QuitLoop();
        } break;
        default: {
          DCHECK(false);
          LOG(ERROR) << " Should Not Reached Here!!!";
        } break;
      }
    } break;
    default:
      DCHECK(false);
      LOG(ERROR) << " Should Not Reached Here!!!";
    break;
  }
}

void MessageLoop::QuitLoop() {
  event_pump_.Quit();
}

int MessageLoop::Notify(int fd, const void* data, size_t count) {
  const static int32_t max_retry_times = 3;
  int ret = 0, retry = 0;

  do {
    ret = ::write(fd, data, count);
    if (ret > 0) {return ret;}

    switch(errno) {
      case EINTR:
      case EAGAIN:
        continue;
      default:
        return ret;
    }
  } while(retry++ < max_retry_times);
  return ret;
}

void MessageLoop::SetThreadNativeName() {
  if (loop_name_.empty()) return;
  pthread_setname_np(pthread_self(), loop_name_.c_str());
}

};
