#ifndef BASE_COROUTINE_SCHEDULER_H_H_
#define BASE_COROUTINE_SCHEDULER_H_H_

#include <cinttypes>
#include <map>
#include <set>
#include <vector>

#include <base/base_micro.h>
#include <base/message_loop/message_loop.h>

#ifdef USE_LIBACO_CORO_IMPL
#include "aco_impl.h"
#else
#include "coro_impl.h"
#endif

namespace base {

/*
 * same thing like go language Goroutine CSP model,
 * But with some aspect diffirence
 *
 * G: CoroTask
 * M: Coroutine
 * P: CoroRunner <-bind-> MessageLoop
 *
 * __go is a utility for schedule a CoroTask(G) Bind To
 * Current(can also bind to a specific native thread) CoroRunner(P)
 *
 * when invoke the G(CoroTask), P(CoroRunner) will choose a suitable
 * M to excute the task
 * */

class CoroRunner : public PersistRunner{
public:
  typedef struct _go {
    _go(const char* func, const char* file, int line)
      : location_(func, file, line),
        target_loop_(_go::loop()) {
    }

    /* specific a loop for this task*/
    inline _go& operator-(MessageLoop* loop) {
      target_loop_ = loop;
      return *this;
    }

    /* schedule a corotine task to remote_queue_, and weakup a
     * target loop to run this task, but this task can be invoke
     * by other loop, if you want task invoke in specific loop
     * use `CO_GO &loop << Functor`
     * */
    template <typename Functor>
    inline void operator-(Functor func) {
      CoroRunner::ScheduleTask(CreateClosure(location_, func));
      target_loop_->WakeUpIfNeeded();
    }

    // here must make sure all things wrapper(copy) into closue,
    // becuase __go object will destruction before task closure run
    template <typename Functor>
    inline void operator<<(Functor closure_fn) {
    	auto func = [=](MessageLoop* loop, const Location& location) {
        CoroRunner& runner = CoroRunner::instance();
        runner.AppendTask(CreateClosure(location, closure_fn));
        // loop->WakeUpIfNeeded(); //see message loop nested task sequence
    	};
      target_loop_->PostTask(location_, func, target_loop_, location_);
    }

    inline static MessageLoop* loop() {
      MessageLoop* current = MessageLoop::Current();
      return  current ? current : CoroRunner::backgroup();
    }
    Location location_;
    base::MessageLoop* target_loop_ = nullptr;
  }_go;

public:
  friend class _go;

  static void Yield();

  static bool Yieldable();

  static LtClosure MakeResumer();

  static void Sleep(uint64_t ms);

  /* here two ways register as runner worker
   * 1. implicit call CoroRunner::instance()
   * 2. call RegisteAsCoroWorker manually
   * */
  static void RegisteAsCoroWorker(MessageLoop*);

  static bool ScheduleTask(TaskBasePtr&& task);
protected:
#ifdef USE_LIBACO_CORO_IMPL
  static void CoroutineEntry();
#else
  static void CoroutineEntry(void *coro);
#endif

  static CoroRunner& instance();

  static MessageLoop* backgroup();

  static ConcurrentTaskQueue remote_queue_;

  CoroRunner();
  ~CoroRunner();

  /* override from MessageLoop::PersistRunner
   * load(bind) task(cargo) to coroutine(car) and run(transfer)
   * */
  void Sched() override ;

  void LoopGone(MessageLoop* loop) override;

  void YieldInternal() {
    SwapCurrentAndTransferTo(main_coro_);
  }

  void AppendTask(TaskBasePtr&& task);

  /* release the coroutine memory */
  void GcAllCachedCoroutine();

  /* append to a cached-list waiting for gc */
  void Append2GCCache(Coroutine* co) {
    cached_gc_coros_.emplace_back(co);
  };

  bool StealingTasks();

  /* check whether still has pending task need to run
   * case 0: still has pending task need to run
   *  return true at once
   * case 1: no pending task need to run
   *    1.1: enough coroutine(task executor)
   *      return false; then corotuine will end its life time
   *    1.2: just paused this corotuine, and wait task to resume it
   *      return true
   */
  bool ContinueRun();

  size_t TaskCount() const {
    return coro_tasks_.size() + remote_queue_.size_approx();
  }

  size_t InQueueTaskCount() const {
    return sched_tasks_.size();
  }

  size_t NoneRemoteTaskCount() const {
    return coro_tasks_.size();
  }

  /* retrieve task from inloop queue or public task pool*/
  bool GetTask(TaskBasePtr& task);

  /* from stash list got a coroutine or create new one*/
  Coroutine* RetrieveCoroutine();

  /* a callback function using for resume a kPaused coroutine */
  void Resume(WeakCoroutine& coro, uint64_t id);

  /* do resume a coroutine from main_coro*/
  void DoResume(WeakCoroutine& coro, uint64_t id);

  bool IsMain() const {return main_coro_ == current_;}

  /* switch call stack from different coroutine*/
  void SwapCurrentAndTransferTo(Coroutine *next);

  std::string RunnerInfo() const;
private:
  Coroutine* current_;
  RefCoroutine main_;
  Coroutine* main_coro_;

  MessageLoop* bind_loop_;

  // executor task list
  std::list<TaskBasePtr> coro_tasks_;

  // scheduled task list
  std::list<TaskBasePtr> sched_tasks_;

  std::vector<Coroutine*> cached_gc_coros_;

  /* every thread max resuable coroutines*/
  size_t max_parking_count_;
  std::list<Coroutine*> parking_coros_;

  /* we hardcode a max cpu preempt time 2ms */
  size_t stealing_counter_ = 0;
  DISALLOW_COPY_AND_ASSIGN(CoroRunner);
};

}// end base

//NOTE: co_yield,co_await,co_resume has been part of keywords in c++20,
// crying!!! so rename co_xxx.... to avoid conflicting
#define CO_GO         ::base::CoroRunner::_go(__FUNCTION__, __FILE__, __LINE__)-
#define CO_YIELD      ::base::CoroRunner::Yield()
#define CO_RESUMER    ::base::CoroRunner::MakeResumer()
#define CO_CANYIELD   ::base::CoroRunner::Yieldable()
#define CO_SLEEP(ms)  ::base::CoroRunner::Sleep(ms)

#endif
