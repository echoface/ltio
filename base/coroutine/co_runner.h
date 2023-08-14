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

#ifndef BASE_COROUTINE_SCHEDULER_H_H_
#define BASE_COROUTINE_SCHEDULER_H_H_

#include <cinttypes>
#include <vector>

#include <base/lt_macro.h>
#include <base/message_loop/message_loop.h>

namespace co {

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
class Coroutine;

class CoroRunner : public base::PersistRunner {
public:
  typedef struct _go {
    _go(const char* func, const char* file, int line)
      : location_(func, file, line),
        loop_(base::MessageLoop::Current()) {}

    /* specific a loop for this task*/
    inline _go& operator-(base::MessageLoop* loop) {
      loop_ = loop;
      return *this;
    }

    /* schedule a corotine task to remote_queue_, and weakup a
     * target loop to run this task, but this task can be invoke
     * by other loop, if you want task invoke in specific loop
     * use `CO_GO &loop << Functor`
     * */
    template <typename Functor>
    inline void operator-(Functor func) {
      CoroRunner::Publish(CreateClosure(location_, func));
      loop()->WakeUpIfNeeded();
    }

    // here must make sure all things wrapper(copy) into closue,
    // becuase __go object will destruction before task closure run
    template <typename Functor>
    inline void operator<<(Functor closure_fn) {
      auto func = [closure_fn](const base::Location& location) {
        CoroRunner& runner = CoroRunner::instance();
        runner.AppendTask(CreateClosure(location, closure_fn));
      };
      loop()->PostTask(location_, func, location_);
    }

    inline base::MessageLoop* loop() {
      return loop_ ? loop_ : CoroRunner::backgroup();
    }
    base::Location location_;
    base::MessageLoop* loop_ = nullptr;
  } _go;

public:
  // run other task if current task occupy cpu too long
  // if task running time > us, runner will give up cpu
  // and then run a task in pending queue
  static void Sched(int64_t us = 5000);

  static void Yield();

  static bool Waitable();

  static base::LtClosure MakeResumer();

  static void Sleep(uint64_t ms);

  static bool Publish(base::TaskBasePtr&& task);

  /* here two ways register as runner worker
   * 1. implicit call CoroRunner::instance()
   * 2. call RegisteRunner manually
   */
  static void RegisteRunner(base::MessageLoop*);

  static base::MessageLoop* BindLoop();

public:
  virtual ~CoroRunner();

  void ScheduleTask(base::TaskBasePtr&& tsk);

  void AppendTask(base::TaskBasePtr&& task);

  size_t TaskCount() const;

protected:
  static CoroRunner& instance();

  static base::MessageLoop* backgroup();

  CoroRunner(base::MessageLoop* loop);

  // scheduled task list
  base::TaskQueue remote_sched_;

  // nested task sched in loop
  std::vector<base::TaskBasePtr> sched_tasks_;

  base::MessageLoop* bind_loop_ = nullptr;

  DISALLOW_COPY_AND_ASSIGN(CoroRunner);
};

}  // namespace co

#define CO_GO co::CoroRunner::_go(__FUNCTION__, __FILE__, __LINE__) -
#define CO_YIELD co::CoroRunner::Yield()
#define CO_RESUMER co::CoroRunner::MakeResumer()
#define CO_CANYIELD co::CoroRunner::Waitable()
#define CO_SLEEP(ms) co::CoroRunner::Sleep(ms)

#define co_go co::CoroRunner::_go(__FUNCTION__, __FILE__, __LINE__) -
#define co_sleep(ms) co::CoroRunner::Sleep(ms)
#define co_new_resumer() co::CoroRunner::MakeResumer()

#define __co_wait_here__ co::CoroRunner::Yield()
#define __co_sched_here__ co::CoroRunner::Sched()
#define __co_yielable__ co::CoroRunner::Waitable()
#define __co_waitable__ co::CoroRunner::Waitable()

// sync task in coroutine context
#define co_sync(func)                                        \
  do {                                                       \
    auto loop = base::MessageLoop::Current();                \
    loop->PostTask(NewClosureWithCleanup(func, CO_RESUMER)); \
    __co_wait_here__;                                        \
  } while (0)

#define CO_SYNC(func) co_sync(func)

#endif
