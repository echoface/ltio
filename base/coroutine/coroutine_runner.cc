
#include <mutex>

#include <base/closure/closure_task.h>
#include <base/memory/lazy_instance.h>

#include "coroutine_runner.h"
#include "glog/logging.h"

namespace base {
static const int kMaxReuseCoroutineNumbersPerThread = 500;

namespace __detail {
  struct _T : public CoroRunner {};
}
//static thread_local base::LazyInstance<__detail::_T> tls_runner = LAZY_INSTANCE_INIT;
static thread_local __detail::_T tls_runner_impl;
static thread_local CoroRunner* tls_runner = NULL;
static std::once_flag backgroup_once;

ConcurrentTaskQueue CoroRunner::stealing_queue;

//IMPORTANT NOTE: NO HEAP MEMORY HERE!!!
#ifdef USE_LIBACO_CORO_IMPL
void CoroRunner::CoroutineEntry() {
  Coroutine* coroutine = static_cast<Coroutine*>(aco_get_arg());
#else
void CoroRunner::CoroutineEntry(void *coro) {
  Coroutine* coroutine = static_cast<Coroutine*>(coro);
#endif

  CoroRunner& runner = CoroRunner::Runner();
  CHECK(coroutine);

  TaskBasePtr task;
  do {
    DCHECK(coroutine->IsRunning());
    if (runner.GetTask(task)) {
      task->Run(); task.reset();
    };
  }while(runner.ContinueRun());

  task.reset();
  coroutine->SetCoroState(CoroState::kDone);
  runner.to_be_delete_.push_back(coroutine);

#ifdef USE_LIBACO_CORO_IMPL
  //libaco aco_exti will transer
  //to main_coro in its inner controller
  runner.current_ = runner.main_coro_;
  coroutine->Exit();
#else
  /* libcoro has the same flow for exit the finished coroutine*/
  runner.SwapCurrentAndTransferTo(runner.main_coro_);
#endif
}


//static
base::MessageLoop* CoroRunner::backgroup() {
  static base::MessageLoop backgroup;
  std::call_once(backgroup_once, [&]() {
    backgroup.SetLoopName("coro_background");
    backgroup.Start();
    //initialized a runner binded to this loop
    backgroup.PostTask(FROM_HERE, &CoroRunner::Runner);
  });
  return &backgroup;
}

//static
bool CoroRunner::schedule_task(TaskBasePtr&& task) {
  return stealing_queue.enqueue(std::move(task));
}

//static
void CoroRunner::RegisteAsCoroWorker(base::MessageLoop* l) {
  l->InstallPersistRunner(&Runner());
}

CoroRunner::CoroRunner()
  : gc_task_scheduled_(false),
    bind_loop_(MessageLoop::Current()),
    max_parking_count_(kMaxReuseCoroutineNumbersPerThread) {

  CHECK(bind_loop_);

  tls_runner = &tls_runner_impl;
  main_ = Coroutine::CreateMain();
  current_ = main_coro_ = main_.get();

  bind_loop_->InstallPersistRunner(this);
  VLOG(GLOG_VINFO) << "CoroutineRunner@" << this << " initialized";
}

CoroRunner::~CoroRunner() {
  FreeOutdatedCoro();
  for (auto coro : stash_list_) {
    coro->ReleaseSelfHolder();
  }
  stash_list_.clear();

  current_ = nullptr;
  main_coro_ = nullptr;
  VLOG(GLOG_VINFO) << "CoroutineRunner@" << this << " gone";
}

CoroRunner& CoroRunner::Runner() {
  return tls_runner_impl;
}

void CoroRunner::ScheduleTask(TaskBasePtr&& task) {
  coro_tasks_.push_back(std::move(task));
}

bool CoroRunner::GetTask(TaskBasePtr& task) {
  if (!coro_tasks_.empty()) {
    task = std::move(coro_tasks_.front());
    coro_tasks_.pop_front();
    return true;
  }
  
  // steal from global task queue
  return stealing_queue.try_dequeue(task);
}

bool CoroRunner::HasMoreTask() const {
  return coro_tasks_.size() || stealing_queue.size_approx();
}

void CoroRunner::Sched() {
  //get a corotuien and start to run
  while (HasMoreTask()) {
    //P(Runner) take a M(Corotine) do work(CoroutineEntry)
    Coroutine* coro = RetrieveCoroutine();
    DCHECK(coro);

    SwapCurrentAndTransferTo(coro);
  }
  FreeOutdatedCoro();
}

void CoroRunner::Yield() {
  if (IsMain()) {
    LOG(ERROR) << __func__ << RunnerInfo() << " main coro can't Yield";
    return;
  }
  VLOG(GLOG_VINFO) << __func__ << RunnerInfo() << " will paused";
  SwapCurrentAndTransferTo(main_coro_);
}

void CoroRunner::Sleep(uint64_t ms) {
  bind_loop_->PostDelayTask(NewClosure(co_resumer()), ms);
  Yield();
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
    if (!coro) {return;}

    coroutine = coro.get();
  }
  if (!coroutine->CanResume(id)) {
    return;
  }
  VLOG(GLOG_VTRACE) << __func__ << RunnerInfo() << " next:" << coroutine;
  return SwapCurrentAndTransferTo(coroutine);
}

StlClosure CoroRunner::Resumer() {
  auto weak_coro = current_->AsWeakPtr();
  uint64_t resume_id = current_->ResumeId();
  return std::bind(&CoroRunner::Resume, this, weak_coro, resume_id);
}

void CoroRunner::SwapCurrentAndTransferTo(Coroutine *next) {
  CHECK(current_ != next);
  Coroutine* current = current_;

  current_ = next;
  current->TransferTo(next);
}

bool CoroRunner::ContinueRun() {
  if (HasMoreTask()) {
    return true;
  }

  // 不需要这个小车了, 结束小车的生命周期
  if (stash_list_.size() > max_parking_count_) {
    return false;
  }

  //Pause here, 将小车进站停车, 等待有客人需要
  //调度时, 再次取回小车并从新从这里发动;
  stash_list_.push_back(current_);
  SwapCurrentAndTransferTo(main_coro_);
  return true;
}

Coroutine* CoroRunner::RetrieveCoroutine() {
  Coroutine* coro = nullptr;
  while(stash_list_.size() && !coro) {
    coro = stash_list_.front();
    stash_list_.pop_front();
  }

  if (coro) {
    return coro;
  }

  auto coro_ptr =
#ifdef USE_LIBACO_CORO_IMPL
    Coroutine::Create(&CoroRunner::CoroutineEntry, main_coro_);
#else
    Coroutine::Create(&CoroRunner::CoroutineEntry);
#endif
  coro_ptr->SelfHolder(coro_ptr);

  return coro_ptr.get();
}

void CoroRunner::FreeOutdatedCoro() {
  for (auto& coro : to_be_delete_) {
    coro->ReleaseSelfHolder();
  }
  to_be_delete_.clear();
  gc_task_scheduled_ = false;
}


std::string CoroRunner::RunnerInfo() const {
  std::ostringstream oss;
  oss << "[ current:" << current_
      << ", wait_id:" << current_->ResumeId()
      << ", status:" << current_->StateToString()
      << ", is_main:" << (IsMain() ? "true" : "false") << "] ";
  return oss.str();
}

}// end base
