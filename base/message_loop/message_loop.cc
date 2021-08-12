/*
 * Copyright 2021 <name of copyright holder>
 * Author: Huan.Gong <gonghuan.dev@gmail.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "message_loop.h"

#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/eventfd.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include <chrono>
#include <csignal>
#include <functional>
#include <iostream>
#include <sstream>
#include <string>

#include "file_util_linux.h"
#include "timeout_event.h"
#include "timer_task_helper.h"

#include <base/base_constants.h>
#include <base/time/time_utils.h>
#include <base/utils/sys_error.h>

#ifndef LOOP_LOG_DETAIL
#define LOOP_LOG_DETAIL "MessageLoop@" << this << "[name:" << loop_name_ << "] "
#endif

namespace base {

namespace {
// thread safe
thread_local MessageLoop* threadlocal_current_ = NULL;

constexpr char kQuit = 1;
constexpr int64_t kTaskFdCounter = 1;

uint64_t generate_loop_id() {
  static std::atomic<uint64_t> _id = {0};
  uint64_t next = _id.fetch_add(1);
  return next == 0 ? _id.fetch_add(1) : next;
}

std::string generate_loop_name() {
  static std::atomic<uint64_t> _id = {0};
  return "loop@" + std::to_string(_id.fetch_add(1));
}

}  // namespace

MessageLoop* MessageLoop::Current() {
  return threadlocal_current_;
}

// static for test ut
uint64_t MessageLoop::GenLoopID() {
  return generate_loop_id();
}

MessageLoop::MessageLoop() : MessageLoop(generate_loop_name()) {}

MessageLoop::MessageLoop(const std::string& name)
  : start_flag_(0),
    loop_name_(name),
    wakeup_pipe_in_(0) {
  start_flag_.clear();
  notify_flag_.clear();

  int fds[2];
  CHECK(CreateLocalNoneBlockingPipe(fds));

  wakeup_pipe_in_ = fds[1];
  wakeup_event_ = FdEvent::Create(this, fds[0], LtEv::READ);

  int ev_fd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
  task_event_ = FdEvent::Create(this, ev_fd, LtEv::READ);

  pump_.SetLoopId(GenLoopID());
}

MessageLoop::~MessageLoop() {
  CHECK(!(running_ && IsInLoopThread()));

  QuitLoop();
  Notify(wakeup_pipe_in_, &kQuit, sizeof(kQuit));

  // first join, others wait util loop end
  if (thread_ptr_ && thread_ptr_->joinable()) {
    VLOG(GLOG_VINFO) << LOOP_LOG_DETAIL << "join here";
    thread_ptr_->join();
  }

  WaitLoopEnd();

  ::close(wakeup_pipe_in_);
  VLOG(GLOG_VINFO) << LOOP_LOG_DETAIL << "Gone";
}

void MessageLoop::WakeUpIfNeeded() {
  if (notify_flag_.test_and_set()) {
    return;
  }
  int fd = task_event_->GetFd();
  if (Notify(fd, &kTaskFdCounter, sizeof(kTaskFdCounter)) <= 0) {
    notify_flag_.clear();
  }
}

void MessageLoop::HandleEvent(FdEvent* fdev, LtEv::Event ev) {
  if (LtEv::has_read(ev)) {
    return HandleRead(fdev);
  }
  LOG(ERROR) << LOOP_LOG_DETAIL << "event:" << fdev->EventInfo();
}

void MessageLoop::HandleRead(FdEvent* fd_event) {
  if (fd_event == task_event_.get()) {
    RunCommandTask(ScheduledTaskType::TaskTypeDefault);
  } else if (fd_event == wakeup_event_.get()) {
    RunCommandTask(ScheduledTaskType::TaskTypeCtrl);
  } else {
    LOG(ERROR) << LOOP_LOG_DETAIL << "should not reached";
  }
}

void MessageLoop::SetLoopName(std::string name) {
  loop_name_ = name;
}

bool MessageLoop::IsInLoopThread() const {
  return pump_.IsInLoop();
}

void MessageLoop::Start() {
  if (start_flag_.test_and_set(std::memory_order_acquire)) {
    LOG(INFO) << LOOP_LOG_DETAIL << "aready start runing...";
    return;
  }

  thread_ptr_.reset(new std::thread(std::bind(&MessageLoop::ThreadMain, this)));
  {
    std::unique_lock<std::mutex> lk(start_stop_lock_);
    cv_.wait(lk, [this]() { return running_; });
  }
}

PersistRunner* MessageLoop::DelegateRunner() {
  return delegate_runner_;
}

void MessageLoop::InstallPersistRunner(PersistRunner* runner) {
  CHECK(IsInLoopThread());
  CHECK(delegate_runner_ == nullptr) << "override runner";
  delegate_runner_ = runner;
}

void MessageLoop::WaitLoopEnd(int32_t ms) {
  if (!running_) {
    return;
  }

  // can't wait stop in loop thread
  CHECK(!IsInLoopThread()) << LOOP_LOG_DETAIL << ", id:" << pump_.LoopID()
                           << ", tid:" << EventPump::CurrentThreadLoopID();
  {
    std::unique_lock<std::mutex> lk(start_stop_lock_);
    cv_.wait(lk, [this]() { return !running_; });
  }
}

uint64_t MessageLoop::PumpTimeout() {
  if (delegate_runner_ && delegate_runner_->HasPeedingTask()) {
    return 0;
  }
  return PendingTasksCount() > 0 ? 0 : 50;
}

size_t MessageLoop::PendingTasksCount() const {
  return in_loop_tasks_.size() + scheduled_tasks_.size_approx();
}

void MessageLoop::ThreadMain() {
  running_ = true;
  threadlocal_current_ = this;

  pump_.PrepareRun();
  CHECK(IsInLoopThread());

  SetThreadNativeName();

  VLOG(GLOG_VINFO) << LOOP_LOG_DETAIL << "Start";

  pump_.InstallFdEvent(task_event_.get());
  pump_.InstallFdEvent(wakeup_event_.get());

  cv_.notify_all();
  while (running_) {
    // pump io/timer event
    pump_.Pump(PumpTimeout());

    RunNestedTask();
  }

  RunNestedTask();

  RunScheduledTask();

  pump_.RemoveFdEvent(task_event_.get());

  pump_.RemoveFdEvent(wakeup_event_.get());

  threadlocal_current_ = NULL;
  VLOG(GLOG_VINFO) << LOOP_LOG_DETAIL << "Thread End";
  cv_.notify_all();
}

bool MessageLoop::PostDelayTask(TaskBasePtr task, uint32_t ms) {
  CHECK(running_) << LOOP_LOG_DETAIL;

  if (!IsInLoopThread()) {
    return PostTask(std::move(NewTimerTaskHelper(std::move(task), Pump(), ms)));
  }

  TimeoutEvent* timeout_ev = TimeoutEvent::CreateOneShot(ms, true);
  timeout_ev->InstallHandler(std::move(task));
  pump_.AddTimeoutEvent(timeout_ev);
  return true;
}

bool MessageLoop::PostTask(TaskBasePtr&& task) {
  CHECK(running_) << LOOP_LOG_DETAIL << task->ClosureInfo();

  if (IsInLoopThread()) {
    in_loop_tasks_.push_back(std::move(task));
    return true;
  }
  bool result = scheduled_tasks_.enqueue(std::move(task));
  WakeUpIfNeeded();
  return result;
}

void MessageLoop::RunScheduledTask() {
  DCHECK(IsInLoopThread());

  TaskBasePtr task;  // instead of pinco *p;
  while (scheduled_tasks_.try_dequeue(task)) {
    task->Run(), task.reset();
  }
}

void MessageLoop::RunNestedTask() {
  DCHECK(IsInLoopThread());

  std::vector<TaskBasePtr> nest_tasks(std::move(in_loop_tasks_));
  for (const auto& task : nest_tasks) {
    task->Run();
  }

  // Note: can't in Sched uninstall runner
  if (delegate_runner_) {
    delegate_runner_->Run();
  }
}

void MessageLoop::RunCommandTask(ScheduledTaskType type) {
  switch (type) {
    case ScheduledTaskType::TaskTypeDefault: {
      uint64_t count = 0;
      int ret = ::read(task_event_->GetFd(), &count, sizeof(count));
      LOG_IF(ERROR, ret < 0)
          << " error:" << StrError() << " fd:" << task_event_->GetFd();

      // clear must clear after read
      notify_flag_.clear();

      RunScheduledTask();

    } break;
    case ScheduledTaskType::TaskTypeCtrl: {
      char buf = 0x7F;
      int ret = ::read(wakeup_event_->GetFd(), &buf, sizeof(buf));
      LOG_IF(ERROR, ret < 0)
          << " error:" << StrError(errno) << " fd:" << wakeup_event_->GetFd();

      switch (buf) {
        case kQuit: {
          QuitLoop();
        } break;
        default: {
          LOG(ERROR) << LOOP_LOG_DETAIL << "should not reached";
          DCHECK(false);
        } break;
      }
    } break;
    default:
      DCHECK(false);
      LOG(ERROR) << LOOP_LOG_DETAIL << "should not reached";
      break;
  }
}

void MessageLoop::QuitLoop() {
  running_ = false;
}

int MessageLoop::Notify(int fd, const void* data, size_t count) {
  return ::write(fd, data, count);
}

void MessageLoop::SetThreadNativeName() {
  if (loop_name_.empty())
    return;
  pthread_setname_np(pthread_self(), loop_name_.c_str());
}

#undef LOOP_LOG_DETAIL

};  // namespace base
