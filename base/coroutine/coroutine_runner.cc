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

#include "coroutine_runner.h"

#include <base/closure/closure_task.h>
#include <base/memory/lazy_instance.h>
#include "glog/logging.h"

#include "coroutine.h"
#ifdef USE_LIBACO_CORO_IMPL
#include "aco_impl.cc"
#else
#include "coro_impl.cc"
#endif

namespace {
const char* kCoSleepWarning =
    "blocking co_sleep is hit, consider move you task into coro";
const size_t kMaxStealOneSched = 1024;
}  // namespace

namespace base {

static std::once_flag backgroup_once;

static thread_local CoroRunner* tls_runner = NULL;

static const int kMaxReuseCoroutineNumbersPerThread = 500;

ConcurrentTaskQueue CoroRunner::remote_queue_;

// IMPORTANT NOTE: NO HEAP MEMORY HERE!!!
#ifdef USE_LIBACO_CORO_IMPL
void CoroRunner::CoroutineEntry() {
  Coroutine* coroutine = static_cast<Coroutine*>(aco_get_arg());
#else
void CoroRunner::CoroutineEntry(void* coro) {
  Coroutine* coroutine = static_cast<Coroutine*>(coro);
#endif

  CHECK(coroutine);
  CoroRunner& runner = instance();
  {
    TaskBasePtr task;
    do {
      coroutine->ResetElapsedTime();
      if (runner.GetTask(task)) {
        task->Run();
        task.reset();
      }
    } while (runner.ContinueRun());
    task.reset();
  }
  coroutine->SetState(CoroState::kDone);
  runner.Append2GCCache(coroutine);

#ifdef USE_LIBACO_CORO_IMPL
  // libaco aco_exti will swapout current and jump back pthread flow
  runner.current_ = runner.main_coro_;
  coroutine->Exit();
#else
  /* libcoro has the same flow for exit the finished coroutine*/
  runner.SwapCurrentAndTransferTo(runner.main_coro_);
#endif
}

CoroRunner& CoroRunner::instance() {
  static thread_local CoroRunner runner;
  return runner;
}

// static
MessageLoop* CoroRunner::backgroup() {
  static LazyInstance<MessageLoop> loop = LAZY_INSTANCE_INIT;
  std::call_once(backgroup_once, [&]() {
    loop->SetLoopName("co_background");
    loop->Start();
    // initialized a runner binded to this loop
    RegisteAsCoroWorker(loop.ptr());
  });
  return loop.ptr();
}

// static
void CoroRunner::RegisteAsCoroWorker(MessageLoop* l) {
  if (l->IsInLoopThread()) {
    instance();
    return;
  }
  l->PostTask(FROM_HERE, &CoroRunner::instance);
}

// static
bool CoroRunner::Yieldable() {
  return tls_runner ? (!tls_runner->IsMain()) : false;
}

// static
void CoroRunner::Sched(int64_t us) {
  if (!tls_runner || tls_runner->IsMain()) {
    return;
  }
  Coroutine* coro = tls_runner->current_;
  if (coro->ElapsedTime() < us || tls_runner->TaskCount() == 0) {
    return;
  }
  MessageLoop* loop = tls_runner->bind_loop_;
  if (loop->PostTask(NewClosure(MakeResumer()))) {
    return tls_runner->SwapCurrentAndTransferTo(tls_runner->main_coro_);
  }
}

// static
void CoroRunner::Yield() {
  CHECK(Yieldable());
  tls_runner->SwapCurrentAndTransferTo(tls_runner->main_coro_);
}

// static
void CoroRunner::Sleep(uint64_t ms) {
  if (!Yieldable()) {
    DCHECK(false) << "CO_SLEEP only work on coro context";
    return;
  }

  MessageLoop* loop = tls_runner->bind_loop_;
  if (loop->PostDelayTask(NewClosure(MakeResumer()), ms)) {
    return tls_runner->SwapCurrentAndTransferTo(tls_runner->main_coro_);
  }

  std::this_thread::yield();
  DCHECK(false) << "CO_SLEEP failed, task schedule failed";
}

// static
LtClosure CoroRunner::MakeResumer() {
  if (!Yieldable()) {
    return nullptr;
  }
  auto weak_coro = tls_runner->current_->AsWeakPtr();
  uint64_t resume_id = tls_runner->current_->ResumeID();
  return std::bind(&CoroRunner::Resume, tls_runner, weak_coro, resume_id);
}

// static
bool CoroRunner::ScheduleTask(TaskBasePtr&& task) {
  return remote_queue_.enqueue(std::move(task));
}

CoroRunner::CoroRunner()
  : bind_loop_(MessageLoop::Current()),
    max_parking_count_(kMaxReuseCoroutineNumbersPerThread) {
  CHECK(bind_loop_);

  tls_runner = this;
  main_ = Coroutine::CreateMain();
  current_ = main_coro_ = main_.get();

  bind_loop_->InstallPersistRunner(this);
  VLOG(GLOG_VINFO) << "CoroutineRunner@" << this << " initialized";
}

CoroRunner::~CoroRunner() {
  GcAllCachedCoroutine();
  for (auto coro : parking_coros_) {
    coro->ReleaseSelfHolder();
  }
  parking_coros_.clear();

  current_ = nullptr;
  main_coro_ = nullptr;
  VLOG(GLOG_VINFO) << "CoroutineRunner@" << this << " gone";
}

bool CoroRunner::StealingTasks() {
  TaskBasePtr task;
  if (stealing_counter_ > kMaxStealOneSched ||
      false == remote_queue_.try_dequeue(task)) {
    return false;
  }

  stealing_counter_++;
  coro_tasks_.push_back(std::move(task));
  return true;
}

bool CoroRunner::GetTask(TaskBasePtr& task) {
  if (coro_tasks_.empty()) {
    return false;
  }
  task = std::move(coro_tasks_.front());
  coro_tasks_.pop_front();
  return true;
}

bool CoroRunner::ContinueRun() {
  if (coro_tasks_.size()) {
    return true;
  }

  if (StealingTasks()) {
    return true;
  }

  // parking this coroutine for next task
  if (parking_coros_.size() < max_parking_count_) {
    parking_coros_.push_back(current_);
    SwapCurrentAndTransferTo(main_coro_);
    return true;
  }

  return false;
}

void CoroRunner::AppendTask(TaskBasePtr&& task) {
  sched_tasks_.push_back(std::move(task));
}

void CoroRunner::LoopGone(MessageLoop* loop) {
  bind_loop_ = nullptr;
  LOG(ERROR) << "loop gone, CoroRunner feel not good";
}

void CoroRunner::Run() {
  stealing_counter_ = 0;
  // after here, any nested task will be handled next loop
  coro_tasks_.swap(sched_tasks_);

  // P(CoroRunner) take a M(Corotine) do work(TaskBasePtr)
  while (TaskCount() > 0) {
    Coroutine* coro = RetrieveCoroutine();
    DCHECK(coro);

    SwapCurrentAndTransferTo(coro);
  }

  GcAllCachedCoroutine();
}

/* 如果本身是在一个子coro里面,
 * 需要在重新将resume调度到MainCoro内
 * 如果本身是MainCoro，则直接进行切换.
 * 如果不是在调度的线程里,则调度到目标Loop去Resume*/
void CoroRunner::Resume(std::weak_ptr<Coroutine>& weak, uint64_t id) {
  if (bind_loop_->IsInLoopThread() && IsMain()) {
    return DoResume(weak, id);
  }
  auto f = std::bind(&CoroRunner::DoResume, this, weak, id);
  CHECK(bind_loop_->PostTask(NewClosure(f)));
}

void CoroRunner::DoResume(WeakCoroutine& weak, uint64_t id) {
  DCHECK(IsMain());

  Coroutine* coroutine = nullptr;
  {
    auto coro = weak.lock();
    if (!coro) {
      return;
    }

    coroutine = coro.get();
  }
  if (!coroutine->CanResume(id)) {
    return;
  }
  return SwapCurrentAndTransferTo(coroutine);
}

void CoroRunner::SwapCurrentAndTransferTo(Coroutine* next) {
  if (next == current_) {
    LOG(INFO) << "coro: next_coro == current, do nothing";
    return std::this_thread::yield();
  }
  Coroutine* current = current_;

  VLOG(GLOG_VTRACE) << __func__ << RunnerInfo() << " switch to next:" << next;

  current_ = next;
  current->TransferTo(next);
}

Coroutine* CoroRunner::RetrieveCoroutine() {
  Coroutine* coro = nullptr;
  while (parking_coros_.size()) {
    coro = parking_coros_.front();
    parking_coros_.pop_front();

    if (coro)
      return coro;
  }

  auto coro_ptr = Coroutine::Create(&CoroRunner::CoroutineEntry, main_coro_);
  coro_ptr->SelfHolder(coro_ptr);

  return coro_ptr.get();
}

void CoroRunner::GcAllCachedCoroutine() {
  for (auto& coro : cached_gc_coros_) {
    coro->ReleaseSelfHolder();
  }
  cached_gc_coros_.clear();
}

std::string CoroRunner::RunnerInfo() const {
  std::ostringstream oss;
  oss << "[ current:" << current_ << ", wait_id:" << current_->ResumeID()
      << ", status:" << StateToString(current_->Status())
      << ", is_main:" << (IsMain() ? "true" : "false") << "] ";
  return oss.str();
}

}  // namespace base
