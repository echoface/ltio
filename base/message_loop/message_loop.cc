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

#include <csignal>
#include <functional>
#include <iostream>
#include <string>

#include "file_util_linux.h"
#include "timeout_event.h"
#include "timer_task_helper.h"

#include <base/logging.h>
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
  : loop_name_(name),
    wakeup_pipe_in_(0) {

  notify_flag_.clear();

  int fds[2];
  CHECK(CreateLocalNoneBlockingPipe(fds));

  wakeup_pipe_in_ = fds[1];
  ctrl_ev_handler_.reset(
      new base::FdEvent::FuncHandler(std::bind(&MessageLoop::HandleCtrlEvent,
                                               this,
                                               std::placeholders::_1,
                                               std::placeholders::_2)));
  wakeup_event_ = FdEvent::Create(ctrl_ev_handler_.get(), fds[0], LtEv::READ);

  int ev_fd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);

  task_ev_handler_.reset(
      new base::FdEvent::FuncHandler(std::bind(&MessageLoop::HandleTaskEvent,
                                               this,
                                               std::placeholders::_1,
                                               std::placeholders::_2)));
  task_event_ = FdEvent::Create(task_ev_handler_.get(), ev_fd, LtEv::READ);

  pump_.SetLoopId(GenLoopID());
}

MessageLoop::~MessageLoop() {
  CHECK(!IsInLoopThread());

  SyncStop();

  // first join, others wait util loop end
  if (thread_ptr_ && thread_ptr_->joinable()) {
    VLOG(VINFO) << LOOP_LOG_DETAIL << "join here";
    thread_ptr_->join();
  }

  ::close(wakeup_pipe_in_);
  VLOG(VINFO) << LOOP_LOG_DETAIL << "Gone";
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

void MessageLoop::HandleTaskEvent(FdEvent* fdev, LtEv::Event ev) {
  uint64_t count = 0;
  int ret = ::read(task_event_->GetFd(), &count, sizeof(count));
  LOG_IF(ERROR, ret < 0) << " error:" << StrError()
                         << " fd:" << task_event_->GetFd();

  // must clear after read, minimum system call times
  notify_flag_.clear();

  RunScheduledTask();
}

void MessageLoop::HandleCtrlEvent(FdEvent* fdev, LtEv::Event ev) {
  char buf = 0x7F;
  int ret = ::read(wakeup_event_->GetFd(), &buf, sizeof(buf));
  LOG_IF(ERROR, ret < 0) << " error:" << StrError(errno)
                         << " fd:" << wakeup_event_->GetFd();

  if (buf == kQuit) {
    running_ = false;
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

PersistRunner* MessageLoop::DelegateRunner() {
  return delegate_runner_;
}

void MessageLoop::InstallPersistRunner(PersistRunner* runner) {
  CHECK(IsInLoopThread());
  CHECK(delegate_runner_ == nullptr) << "override runner";
  delegate_runner_ = runner;
}

void MessageLoop::Start() {
  auto once_fn = [&]() {
    auto loop_main = std::bind(&MessageLoop::ThreadMain, this);
    thread_ptr_.reset(new std::thread(loop_main));

    std::unique_lock<std::mutex> lk(start_stop_lock_);
    cv_.wait(lk, [this]() { return running_; });
  };
  std::call_once(start_onece_, once_fn);
}

void MessageLoop::SyncStop() {
  CHECK(!IsInLoopThread());

  Notify(wakeup_pipe_in_, &kQuit, sizeof(kQuit));

  WaitLoopEnd();
}

void MessageLoop::QuitLoop() {
  Notify(wakeup_pipe_in_, &kQuit, sizeof(kQuit));
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

uint64_t MessageLoop::CalcMaxPumpTimeout() {
  if (in_loop_tasks_.size() > 0 || scheduled_tasks_.size_approx() > 0) {
    return 0;
  }
  if (delegate_runner_ && delegate_runner_->HasPeedingTask()) {
    return 0;
  }
  return 50;
}

void MessageLoop::ThreadMain() {
  running_ = true;
  threadlocal_current_ = this;

  pump_.PrepareRun();
  CHECK(IsInLoopThread());

  SetThreadNativeName();

  VLOG(VINFO) << LOOP_LOG_DETAIL << "Start";

  pump_.InstallFdEvent(task_event_.get());
  pump_.InstallFdEvent(wakeup_event_.get());

  cv_.notify_all();  // notify loop start; see ::Start
  while (running_) {
    // pump io/timer event; trigger RunScheduledTask
    pump_.Pump(CalcMaxPumpTimeout());

    RunNestedTask();
  }

  RunScheduledTask();

  RunNestedTask();

  pump_.RemoveFdEvent(task_event_.get());

  pump_.RemoveFdEvent(wakeup_event_.get());

  threadlocal_current_ = NULL;
  VLOG(VINFO) << LOOP_LOG_DETAIL << "Thread End";
  cv_.notify_all();  // notify loop stop; see ::WaitLoopEnd
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

int MessageLoop::Notify(int fd, const void* data, size_t count) {
  return ::write(fd, data, count);
}

void MessageLoop::SetThreadNativeName() {
  if (loop_name_.empty())
    return;
  pthread_setname_np(pthread_self(), loop_name_.c_str());
}

RawLoopList RawLoopsFromBundles(const RefLoopList& loops) {
  RawLoopList vec;
  for (auto& l : loops) {
    vec.push_back(l.get());
  }
  return vec;
}

RefLoopList NewLoopBundles(const std::string& prefix, int num) {
  RefLoopList vec;
  for (int i = 0; i < num; i++) {
    RefMessageLoop loop(new MessageLoop(prefix + std::to_string(i)));
    loop->Start();
    vec.push_back(std::move(loop));
  }
  return vec;
}

#undef LOOP_LOG_DETAIL

};  // namespace base
