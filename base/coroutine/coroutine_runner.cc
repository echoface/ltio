
#include "glog/logging.h"
#include <base/closure/closure_task.h>
#include <base/memory/lazy_instance.h>

#include "coroutine_runner.h"

namespace base {
static const int kMaxReuseCoroutineNumbersPerThread = 500;

namespace __detail {
  struct _T : public CoroRunner {};
}
//static thread_local base::LazyInstance<__detail::_T> tls_runner = LAZY_INSTANCE_INIT;
static thread_local __detail::_T tls_runner_impl;
static thread_local CoroRunner* tls_runner = NULL;

//IMPORTANT NOTE: NO HEAP MEMORY HERE!!!

#ifdef USE_LIBACO_CORO_IMPL
void CoroRunner::CoroutineMain() {
  Coroutine* coroutine = static_cast<Coroutine*>(aco_get_arg());
#else
void CoroRunner::CoroutineMain(void *coro) {
  Coroutine* coroutine = static_cast<Coroutine*>(coro);
#endif

  CHECK(coroutine);

  CoroRunner& runner = CoroRunner::Runner();
  bool continue_run = true;

  do {
    DCHECK(coroutine->IsRunning());
    if (runner.coro_tasks_.size()) {
      TaskBasePtr task = std::move(runner.coro_tasks_.front());
      runner.coro_tasks_.pop_front();
      task->Run();
    }
  }while(runner.WaitPendingTask());

  runner.GcCoroutine(coroutine);
}

//clean up coroutine and push to delete list
void CoroRunner::GcCoroutine(Coroutine* coro) {
  CHECK(main_coro_ != coro);

  coro->SetCoroState(CoroState::kDone);
  to_be_delete_.push_back(coro);
  if (!gc_task_scheduled_) {
    bind_loop_->PostTask(NewClosure(std::bind(&CoroRunner::DestroyCroutine, this)));
    gc_task_scheduled_ = true;
  }

#ifdef USE_LIBACO_CORO_IMPL
  //aco_exit(); //libaco aco_exti will transer to main_coro in its inner ctroller
  current_ = main_coro_;
  coro->Exit();
#else
  /* libcoro has the same flow for exit the finished coroutine
   * */
  SwapCurrentAndTransferTo(main_coro_);
#endif
}

CoroRunner::CoroRunner()
  : gc_task_scheduled_(false),
    bind_loop_(MessageLoop::Current()),
    max_reuse_coroutines_(kMaxReuseCoroutineNumbersPerThread) {
  tls_runner = &tls_runner_impl;

  RefCoroutine coro_ptr = Coroutine::CreateMain();
  coro_ptr->SelfHolder(coro_ptr);

  current_ = main_coro_ = coro_ptr.get();

  LOG_IF(ERROR, !bind_loop_) << __FUNCTION__ << " CoroRunner need constructor with initialized loop";
  CHECK(bind_loop_);
}

CoroRunner::~CoroRunner() {
  LOG(INFO) << __FUNCTION__ << " coroutine runner gone";
  DestroyCroutine();
  for (auto coro : stash_list_) {
    coro->ReleaseSelfHolder();
  }
  stash_list_.clear();

  current_->ReleaseSelfHolder();
  main_coro_->ReleaseSelfHolder();
  current_ = nullptr;
  main_coro_ = nullptr;
}

CoroRunner& CoroRunner::Runner() {
  return tls_runner_impl;
}

void CoroRunner::Yield() {
  if (InMainCoroutine()) {
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
  if (bind_loop_->IsInLoopThread() && InMainCoroutine()) {
    return DoResume(weak, id, 1);
  }
  auto f = std::bind(&CoroRunner::DoResume, this, weak, id, 2);
  CHECK(bind_loop_->PostTask(NewClosure(f)));
}

void CoroRunner::DoResume(WeakCoroutine& weak, uint64_t id, int type) {
  DCHECK(InMainCoroutine());

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

void CoroRunner::DestroyCroutine() {
  for (auto& coro : to_be_delete_) {
    coro->ReleaseSelfHolder();
  }
  to_be_delete_.clear();
  gc_task_scheduled_ = false;
}

void CoroRunner::RunCoroutine() {
  while (coro_tasks_.size() > 0) {
    //M
    Coroutine* coro = RetrieveCoroutine();
    CHECK(coro);

    // 只有当这个corotine 在task运行的过程中换出(paused)，那么才会重新回到这里,否则是在task运行完成之后
    // 会调用RecallCoroutine，在依旧有task需要运行的时候，可以设置task， 让这个coro继续
    // 执行task，而不需要切换task，如果没有task， 则将coro回收或者标记成gc状态，等待gc回收
    //
    // 当运行的task在一半的状态被切换出去，回到这里， 如果仍然有task需要调度， 则找一个新的
    // coro运行task，结束循环回到messageloop循环，等待后后续的Task
    //
    // 当yield的之后的task重新被标记成可运行，那么做coro的切换就无法避免了，直接使用Resume
    // 切换调度, 此时调度结束后调用GcCorotuine，task队列中应该会是空的
    SwapCurrentAndTransferTo(coro);
  }
  invoke_coro_shceduled_ = false;
}

bool CoroRunner::WaitPendingTask() {
  if (coro_tasks_.size()) {
    return true;
  }

  if (stash_list_.size() < max_reuse_coroutines_) {

    stash_list_.push_back(current_);
    SwapCurrentAndTransferTo(main_coro_);
    return true;
  }

  return false;
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
    Coroutine::Create(&CoroRunner::CoroutineMain, main_coro_);
#else
    Coroutine::Create(&CoroRunner::CoroutineMain);
#endif
  coro_ptr->SelfHolder(coro_ptr);

  return coro_ptr.get();
}

std::string CoroRunner::RunnerInfo() const {
  std::ostringstream oss;
  oss << "[ current:" << current_
      << ", wait_id:" << current_->ResumeId()
      << ", status:" << current_->StateToString()
      << ", is_main:" << (InMainCoroutine() ? "true" : "false") << "] ";
  return oss.str();
}

}// end base
