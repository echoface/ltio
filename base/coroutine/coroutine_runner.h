#ifndef BASE_COROUTINE_SCHEDULER_H_H_
#define BASE_COROUTINE_SCHEDULER_H_H_

#include <map>
#include <set>
#include <vector>
#include <cinttypes>
#include <base/base_micro.h>
#include <base/message_loop/message_loop.h>

#ifdef USE_LIBACO_CORO_IMPL
#include "aco_impl.h"
#else
#include "coro_impl.h"
#endif

namespace base {

typedef std::function<void()> CoroResumer;

/*
 * same thing like go language Goroutine CSP model,
 * But with some aspect diffirence
 *
 * G: CoroTask
 * M: Coroutine
 * P: CoroRunner
 *
 * __go is a utility for schedule a CoroTask(G) Bind To
 * Current(can also bind to a specific native thread) CoroRunner(P)
 *
 * when invoke the G(CoroTask), P(CoroRunner) will choose a suitable
 * M to excute the task
 * */

class CoroRunner {
public:
  typedef struct _go {
    _go(const char* func, const char* file, int line)
      : location_(func, file, line),
        target_loop_(MessageLoop::Current()) {
    }

    inline _go& operator-(base::MessageLoop* loop) {
      target_loop_ = loop;
      return *this;
    }

    template <typename Functor>
    inline void operator-(Functor arg) {

      CoroRunner& runner = Runner();
      runner.coro_tasks_.push_back(std::move(CreateClosure(location_, arg)));
      if (!runner.invoke_coro_shceduled_ && target_loop_) {
        target_loop_->PostTask(NewClosure(std::bind(&CoroRunner::RunCoroutine, &runner)));
      }
    }

    // here must make sure all things wraper(copy) into closue,
    // becuase __go object will destruction before task closure run
    template <typename Functor>
    inline void operator<<(Functor closure_fn) {
    	auto func = [=](base::MessageLoop* loop, const Location& location) {
        CoroRunner& runner = Runner();
        runner.coro_tasks_.push_back(std::move(CreateClosure(location, closure_fn)));
        if (!runner.invoke_coro_shceduled_ && loop) {
          loop->PostTask(NewClosure(std::bind(&CoroRunner::RunCoroutine, &runner)));
        }
    	};
      target_loop_->PostTask(NewClosure(std::bind(func, target_loop_, location_)));
    }
    Location location_;
    base::MessageLoop* target_loop_;
  }_go;

public:
  static CoroRunner& Runner();

  /* make current coro give up cpu*/
  void Yield();

  /* give up cpu till ms pass*/
  void Sleep(uint64_t ms);

  StlClosure Resumer();

  std::string RunnerInfo() const;

  /* judge wheather running in a main coroutine with thread*/
  bool InMainCoroutine() const { return (main_coro_ == current_);}

  bool CanYieldHere() const {return !InMainCoroutine();}
protected:
  CoroRunner();
  ~CoroRunner();
  /* reuse coroutine or delete*/
  void GcCoroutine(Coroutine* coro);

  /* release the coroutine memory */
  void DestroyCroutine();

  /* install coroutine to P(CoroRunner, binded to a native thread)*/
  void RunCoroutine();

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

  /* from stash list got a coroutine or create new one*/
  Coroutine* RetrieveCoroutine();

  /* a callback function using for resume a kPaused coroutine */
  void Resume(WeakCoroutine& coro, uint64_t id);

  /* do resume a coroutine from main_coro*/
  void DoResume(WeakCoroutine& coro, uint64_t id);

#ifdef USE_LIBACO_CORO_IMPL
  static void CoroutineMain();
#else
  static void CoroutineMain(void *coro);
#endif

  /* switch call stack from different coroutine*/
  void SwapCurrentAndTransferTo(Coroutine *next);
private:
  Coroutine* current_;
  RefCoroutine main_;
  Coroutine* main_coro_;

  bool gc_task_scheduled_;
  MessageLoop* bind_loop_;

  bool invoke_coro_shceduled_;

  std::list<TaskBasePtr> coro_tasks_;
  std::vector<Coroutine*> to_be_delete_;

  /*every thread max resuable coroutines*/
  size_t max_reuse_coroutines_;
  std::list<Coroutine*> stash_list_;

  DISALLOW_COPY_AND_ASSIGN(CoroRunner);
};

}// end base

//NOTE: co_yield,co_await,co_resume has bee a private key words for c++20,
//so cry to re-name aco_xxx....
#define co_go        ::base::CoroRunner::_go(__FUNCTION__, __FILE__, __LINE__)-
#define co_pause     ::base::CoroRunner::Runner().Yield()
#define co_resumer() ::base::CoroRunner::Runner().Resumer()
#define co_sleep(ms) ::base::CoroRunner::Runner().Sleep(ms)
#define co_can_yield \
  (base::MessageLoop::Current() && ::base::CoroRunner::Runner().CanYieldHere())

#endif
