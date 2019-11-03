#ifndef BASE_COROUTINE_SCHEDULER_H_H_
#define BASE_COROUTINE_SCHEDULER_H_H_

#include <map>
#include <set>
#include <vector>
#include <cinttypes>
#include "base_micro.h"
#include "coroutine/coroutine.h"
#include <base/message_loop/message_loop.h>

namespace base {

typedef std::function<void()> CoroResumer;

class CoroRunner {
public:
  typedef struct __go {
    __go(const char* func, const char* file, int line)
      : location_(func, file, line) {
    }

    template <typename Functor>
    inline void operator-(Functor arg) {

      CoroRunner& runner = Runner();
      runner.coro_tasks_.push_back(std::move(CreateClosure(location_, arg)));
      if (!runner.invoke_coro_shceduled_ && target_loop_) {
        target_loop_->PostTask(NewClosure(std::bind(&CoroRunner::InvokeCoroutineTasks, &runner)));
      }
    }
    inline __go& operator-(base::MessageLoop* loop) {
      target_loop_ = loop;
      return *this;
    }
    // here must make sure all things wraper(copy) into closue,
    // becuase __go object will destruction before task closure run
    template <typename Functor>
    inline void operator<<(Functor closure_fn) {
    	auto func = [=](base::MessageLoop* loop, const Location& location) {
        CoroRunner& runner = Runner();
        runner.coro_tasks_.push_back(std::move(CreateClosure(location, closure_fn)));
        if (!runner.invoke_coro_shceduled_ && loop) {
          loop->PostTask(NewClosure(std::bind(&CoroRunner::InvokeCoroutineTasks, &runner)));
        }
    	};
      target_loop_->PostTask(NewClosure(std::bind(func, target_loop_, location_)));
    }
    Location location_;
    base::MessageLoop* target_loop_ = MessageLoop::Current();
  }__go;

public:
  static bool CanYield();
  static CoroRunner& Runner();
  /* give up cpu; main coro will automatic resume;
   * other coro should resume by manager*/
  static void YieldCurrent();
  static StlClosure CurrentCoroResumeCtx();

  void SleepMillsecond(uint64_t ms);
  intptr_t CurrentCoroutineId() const;
protected:
  CoroRunner();
  ~CoroRunner();
  /* swich call stack from different coroutine*/
  void TransferTo(Coroutine *next);
  /* reuse: insert to freelist or gc: delete for free memory; then switch to thread coroutine*/
  void GcCoroutine();
  /* release the coroutine memory */
  void ReleaseExpiredCoroutine();
  /* case 1: scheduled_task exsist, set task and run again
   * case 2: scheduled_task empty, recall to freelist or gc it */
  void StashIfNeeded();
  /* a callback function using for resume a kPaused coroutine */
  void ResumeCoroutine(std::weak_ptr<Coroutine> coro, uint64_t id);

  /* judge wheather running in a main coroutine with thread*/
  bool InMainCoroutine() const { return (main_coro_ == current_);}

  void InvokeCoroutineTasks();

  Coroutine* RetrieveCoroutine();

  std::string RunnerInfo() const;
  static void CoroutineMain(void *coro);
private:
  void do_resume(std::weak_ptr<Coroutine> coro, uint64_t id, int type);
  TaskBasePtr retrieve_task();
  Coroutine* current_;
  Coroutine* main_coro_;

  StlClosure gc_task_;
  bool gc_task_scheduled_;
  MessageLoop* bind_loop_;

  bool invoke_coro_shceduled_;

  TaskBasePtr cur_task_;
  std::list<TaskBasePtr> coro_tasks_;
  std::vector<Coroutine*> expired_coros_;

  /*every thread max resuable coroutines*/
  size_t max_reuse_coroutines_;
  std::list<Coroutine*> free_list_;

  DISALLOW_COPY_AND_ASSIGN(CoroRunner);
};

#define co_go        base::CoroRunner::__go(__FUNCTION__, __FILE__, __LINE__)-
#define co_yield     base::CoroRunner::YieldCurrent()
#define co_resumer   base::CoroRunner::CurrentCoroResumeCtx()
#define co_sleep(ms) base::CoroRunner::Runner().SleepMillsecond(ms)

}// end base
#endif
