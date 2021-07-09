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

#include "co_runner.h"

#include <base/closure/closure_task.h>
#include <base/memory/lazy_instance.h>
#include "bstctx_impl.hpp"
#include "glog/logging.h"

namespace {
using TaskList = std::vector<base::TaskBasePtr>;

std::once_flag backgroup_once;

constexpr size_t kMaxStealOneSched = 64;

constexpr size_t kMinReuseCoros = 64;
constexpr size_t kMillsPerSec = 1000;

base::TaskQueue g_remote;

class OneRunContext final {
public:
  OneRunContext() { Reset(); };

  void Reset() {
    task_idx = 0;
    total_cnt = 0;
    tasks.clear();

    max_dequeue = 0;
    tsk_queue = nullptr;
  }

  bool PopTask(base::TaskBasePtr& task) {
    while (task_idx < total_cnt) {
      task = std::move(tasks[task_idx++]);
      if (task)
        return true;
    }
    while (max_dequeue > 0) {
      if (tsk_queue->try_dequeue(task)) {
        max_dequeue--;
        return true;
      }
      max_dequeue = 0;
    }
    return false;
  }

  size_t Remain() const { return (total_cnt - task_idx) + max_dequeue; }

  size_t PumpFromQueue(base::TaskQueue& tq, size_t batch) {
    tq.try_dequeue_bulk(std::back_inserter(tasks), batch);
    return total_cnt = tasks.size();
  }

  size_t SwapFromTaskList(::TaskList& list) {
    tasks.swap(list);
    return total_cnt = tasks.size();
  }

  size_t WithQueueTillNow(base::TaskQueue* tq) {
    tsk_queue = tq;
    max_dequeue = tq->size_approx();
    return max_dequeue;
  }

  size_t WithLimitedDequeue(base::TaskQueue* tq, size_t max) {
    tsk_queue = tq;
    max_dequeue = max;
    return max_dequeue;
  }

private:
  size_t task_idx;
  size_t total_cnt;
  ::TaskList tasks;

  size_t max_dequeue;
  base::TaskQueue* tsk_queue;

  DISALLOW_COPY_AND_ASSIGN(OneRunContext);
};

}  // namespace

namespace base {

class CoroRunnerImpl;
thread_local base::CoroRunnerImpl* g = NULL;

template <typename Rec>
fcontext_transfer_t context_exit(fcontext_transfer_t from) noexcept {
  Rec* rec = static_cast<Rec*>(from.data);
  return from;
}

/* Recorder is a coroutine control struct that represent
 * some coro_context's infomation use for context switching
 *
 * Recorder::RecordCtx(fcontext_transfer_t from)
 * Recorder::CoroMain();
 * Recorder::ExitCurrent();
 * */
template <typename Rec>
void context_entry(fcontext_transfer_t from) noexcept {
  Rec* rec = static_cast<Rec*>(from.data);
  CHECK(rec);
  rec->RecordCtx(from);
LOOP:
  rec->CoroMain();

  rec->ExitCurrent();

  /* this make a coroutine can reuse without reset callstack, just
   * call co->Resume() to reuse this coro context, but!, after testing
   * reset a call stack can more fast than reuse current stack status
   * see: Coroutine::Reset(fn), so when use parking coro, we reset it,
   * instead of just call coro->Resume(); about 1/15 perfomance improve*/
  goto LOOP;
}

class CoroRunnerImpl : public CoroRunner {
public:
  friend void context_entry<CoroRunnerImpl>(fcontext_transfer_t from) noexcept;
  CoroRunnerImpl() : CoroRunner(base::MessageLoop::Current()) {
    g = this;
    bind_loop_->InstallPersistRunner(this);
    current_ = main_ = Coroutine::New()->SelfHolder();

    max_reuse_co_ = 64;
    sched_ticks_ = base::time_ms();
  }

  ~CoroRunnerImpl() {
    FreeCoros();
    Coroutine* co = nullptr;
    while ((co = freelist_.First())) {
      freelist_.Remove(co);
      co->ReleaseSelfHolder();
    }
    main_->ReleaseSelfHolder();
    current_ = nullptr;
  }

  void RecordCtx(fcontext_transfer_t co_zero) {
    main_->co_ctx_.ctx = co_zero.ctx;
  }

  void CoroMain() {
    do {
      TaskBasePtr task;
      while (run_.PopTask(task)) {
        task->Run();
      }
      task.reset();  // ensure task destruction
    } while (0);
  }

  void ExitCurrent() {
    GC(current_);
    SwitchContext(main_);
  }

  /* switch call stack from different coroutine*/
  // NOTE: ensure without any heap memory inside this
  void SwitchContext(Coroutine* next) {
    DCHECK(next != current_)
        << current_->CoInfo() << " next:" << next->CoInfo();
    current_ = next;
    next->Resume();
  }

  /* a callback function using for resume a kPaused coroutine */

  /* 如果本身是在一个子coro里面,
   * 需要在重新将resume调度到MainCoro内
   * 如果本身是MainCoro，则直接进行切换.
   * 如果不是在调度的线程里,则调度到目标Loop去Resume*/
  void Resume(std::weak_ptr<Coroutine>& weak, uint64_t id) {
    if (bind_loop_->IsInLoopThread()) {
      RefCoroutine coro = weak.lock();
      if (coro && coro->ResumeID() == id && !coro->Attatched()) {
        return ready_list_.Append(coro.get());
      }
      LOG(ERROR) << "already resumed, want:" << id
                 << " real:" << coro->ResumeID();
      bind_loop_->WakeUpIfNeeded();
      return;
    }

    /*do resume a coroutine in other thread*/
    auto do_resume = [weak, id, this]() {
      CHECK(bind_loop_->IsInLoopThread());
      RefCoroutine coro = weak.lock();
      if (coro && coro->ResumeID() == id && !coro->Attatched()) {
        return ready_list_.Append(coro.get());
      }
      LOG(ERROR) << "already resumed, want:" << id
                 << " real:" << coro->ResumeID();
    };
    remote_sched_.enqueue(NewClosure(do_resume));
    bind_loop_->WakeUpIfNeeded();
  }

  void GCCurrent() {
    CHECK(current_ != main_);
    return GC(current_);
  }

  /* append to a cached-list waiting for gc */
  void GC(Coroutine* co) {
    CHECK(!co->Attatched());

    if (freelist_.size() < max_reuse_co_) {
      freelist_.Append(co);
      return;
    }
    gc_list_.Append(co);
  }

  /* release && destroy the coroutine*/
  void FreeCoros() {
    Coroutine* co = nullptr;
    while ((co = gc_list_.First())) {
      gc_list_.Remove(co);
      co->ReleaseSelfHolder();
    }
  }

  /* override from MessageLoop::PersistRunner
   * load(bind) task(cargo) to coroutine(car) and run(transfer)
   * P(CoroRunner) take a M(Corotine) do work(TaskBasePtr)
   */
  void Run() override {
    VLOG_EVERY_N(GLOG_VTRACE, 1000) << " coroutine runner enter";

    size_t co_usage_cnt = 0;

    // resume any ready coros
    Coroutine* co = nullptr;
    while ((co = ready_list_.First())) {
      ready_list_.Remove(co);
      SwitchContext(co);
      FreeCoros();
    }

    // ====> run scheduled task
    g->run_.Reset();
    g->run_.WithQueueTillNow(&remote_sched_);
    while (g->run_.Remain()) {
      co_usage_cnt++;
      SwitchContext(Spawn());
      FreeCoros();
    }

    // ====>    run scheduled nested tasks
    g->run_.Reset();
    g->run_.SwapFromTaskList(sched_tasks_);
    while (g->run_.Remain()) {
      co_usage_cnt++;
      SwitchContext(Spawn());
      FreeCoros();
    }

    // ====>    steal remote task
    g->run_.Reset();
    g->run_.WithQueueTillNow(&g_remote);
    while (g->run_.Remain()) {
      co_usage_cnt++;
      SwitchContext(Spawn());
      FreeCoros();
    }

    // resume any ready coros
    while ((co = ready_list_.First())) {
      ready_list_.Remove(co);
      SwitchContext(co);
      FreeCoros();
    }

    peek_co_actives_ = std::max(peek_co_actives_, co_usage_cnt);
    auto ticks = base::time_ms();
    if (ticks - sched_ticks_ > kMillsPerSec) {
      sched_ticks_ = ticks;
      max_reuse_co_ = std::max(kMinReuseCoros, peek_co_actives_);
    }
  }

  /* override from MessageLoop::PersistRunner*/
  void LoopGone(MessageLoop* loop) override {
    bind_loop_ = nullptr;
    LOG(ERROR) << "loop gone, CoroRunner feel not good";
  }

  /* more task or coroutine need run*/
  bool HasPeedingTask() const override {
    return TaskCount() > 0 || !ready_list_.empty();
  }

  Coroutine* Spawn() {
    Coroutine* coro = nullptr;
    while ((coro = freelist_.First())) {
      freelist_.Remove(coro);
      // reset the coro context stack make next time resume more fast
      // bz when context switch can with out copy any data, just jump
      // coro->Reset(context_entry<CoroRunnerImpl>, this);
      return coro;
    }
    return Coroutine::New(context_entry<CoroRunnerImpl>, this)->SelfHolder();
  }

  bool in_main_coro() const { return current_ == main_; }

  Coroutine* main_;

  Coroutine* current_;

  OneRunContext run_;

  // resume task pending to this
  LinkedList<Coroutine> ready_list_;

  LinkedList<Coroutine> freelist_;

  LinkedList<Coroutine> gc_list_;

  // dynamic reuse coroutine number
  size_t max_reuse_co_ = 64;
  size_t sched_ticks_ = 0;
  size_t peek_co_actives_ = 0;
};  // end CoroRunnerImpl

// static
CoroRunner& CoroRunner::instance() {
  static thread_local CoroRunnerImpl runner;
  return runner;
}

// static
MessageLoop* CoroRunner::backgroup() {
  static LazyInstance<MessageLoop> loop = LAZY_INSTANCE_INIT;
  std::call_once(backgroup_once, [&]() {
    loop->SetLoopName("co_background");
    loop->Start();
    // initialized a runner binded to this loop
    RegisteRunner(loop.ptr());
  });
  return loop.ptr();
}

// static
void CoroRunner::RegisteRunner(MessageLoop* l) {
  if (l->IsInLoopThread()) {
    instance();
    return;
  }
  l->PostTask(FROM_HERE, &CoroRunner::instance);
}

// static
bool CoroRunner::Yieldable() {
  return g ? (!g->in_main_coro()) : false;
}

// static
void CoroRunner::Sched(int64_t us) {
  if (!Yieldable()) {
    return;
  }
  if (!g->HasPeedingTask()) {
    return;
  }
  g->AppendTask(NewClosure(MakeResumer()));

  g->SwitchContext(g->main_);
}

// static
void CoroRunner::Yield() {
  CHECK(Yieldable());
  // add current_ into a linkedlist to tracking
  // system-range coroutine status
  g->SwitchContext(g->main_);
}

// static
void CoroRunner::Sleep(uint64_t ms) {
  CHECK(Yieldable()) << "co_sleep only work on coro context";

  MessageLoop* loop = g->bind_loop_;

  CHECK(loop->PostDelayTask(NewClosure(MakeResumer()), ms));

  g->SwitchContext(g->main_);
}

// static
LtClosure CoroRunner::MakeResumer() {
  if (!Yieldable()) {
    LOG(WARNING) << "make resumer on main_coro";
    return nullptr;
  }
  auto weak = g->current_->AsWeakPtr();
  uint64_t resume_id = g->current_->ResumeID();
  return std::bind(&CoroRunnerImpl::Resume, g, weak, resume_id);
}

// static publish a task for any runner
bool CoroRunner::Publish(TaskBasePtr&& task) {
  return g_remote.enqueue(std::move(task));
}

CoroRunner::CoroRunner(MessageLoop* loop) : bind_loop_(loop) {
  CHECK(bind_loop_);
  VLOG(GLOG_VINFO) << "CoroutineRunner@" << this << " initialized";
}

CoroRunner::~CoroRunner() {
  VLOG(GLOG_VINFO) << "CoroutineRunner@" << this << " gone";
}

void CoroRunner::ScheduleTask(TaskBasePtr&& tsk) {
  if (bind_loop_->IsInLoopThread()) {
    sched_tasks_.push_back(std::move(tsk));
  } else {
    remote_sched_.enqueue(std::move(tsk));
  }
}

void CoroRunner::AppendTask(TaskBasePtr&& task) {
  CHECK(bind_loop_->IsInLoopThread());
  sched_tasks_.push_back(std::move(task));
}

size_t CoroRunner::TaskCount() const {
  return sched_tasks_.size() + g_remote.size_approx() +
         remote_sched_.size_approx();
}

}  // namespace base
