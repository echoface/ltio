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

#include <base/lt_micro.h>
#include <base/message_loop/message_loop.h>

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
class Coroutine;

class CoroRunner : public PersistRunner {
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
      base::CoroRunner::Publish(CreateClosure(location_, func));
      loop()->WakeUpIfNeeded();
    }

    // here must make sure all things wrapper(copy) into closue,
    // becuase __go object will destruction before task closure run
    template <typename Functor>
    inline void operator<<(Functor closure_fn) {
      auto func = [closure_fn](const base::Location& location) {
        base::CoroRunner& runner = base::CoroRunner::instance();
        runner.AppendTask(CreateClosure(location, closure_fn));
      };
      loop()->PostTask(location_, func, location_);
    }

    inline base::MessageLoop* loop() {
      return loop_ ? loop_ : base::CoroRunner::backgroup();
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

  static LtClosure MakeResumer();

  static void Sleep(uint64_t ms);

  static bool Publish(TaskBasePtr&& task);

  /* here two ways register as runner worker
   * 1. implicit call CoroRunner::instance()
   * 2. call RegisteRunner manually
   */
  static void RegisteRunner(MessageLoop*);

  static MessageLoop* BindLoop();
public:
  virtual ~CoroRunner();

  void ScheduleTask(TaskBasePtr&& tsk);

  void AppendTask(TaskBasePtr&& task);

  size_t TaskCount() const;

protected:
  static CoroRunner& instance();

  static MessageLoop* backgroup();

  CoroRunner(MessageLoop* loop);

  // scheduled task list
  TaskQueue remote_sched_;

  // nested task sched in loop
  std::vector<TaskBasePtr> sched_tasks_;

  MessageLoop* bind_loop_ = nullptr;

  DISALLOW_COPY_AND_ASSIGN(CoroRunner);
};

}  // namespace base

#define CO_GO ::base::CoroRunner::_go(__FUNCTION__, __FILE__, __LINE__) -
#define CO_YIELD ::base::CoroRunner::Yield()
#define CO_RESUMER ::base::CoroRunner::MakeResumer()
#define CO_CANYIELD ::base::CoroRunner::Waitable()
#define CO_SLEEP(ms) ::base::CoroRunner::Sleep(ms)

#define co_go base::CoroRunner::_go(__FUNCTION__, __FILE__, __LINE__) -
#define co_sleep(ms) base::CoroRunner::Sleep(ms)
#define co_new_resumer() base::CoroRunner::MakeResumer()

#define __co_wait_here__ base::CoroRunner::Yield()
#define __co_sched_here__ base::CoroRunner::Sched()
#define __co_yielable__ base::CoroRunner::Waitable()
#define __co_waitable__ base::CoroRunner::Waitable()

// sync task in coroutine context
#define co_sync(func)                                        \
  do {                                                       \
    auto loop = base::MessageLoop::Current();                \
    loop->PostTask(NewClosureWithCleanup(func, CO_RESUMER)); \
    __co_wait_here__;                                        \
  } while (0)

#define CO_SYNC(func) co_sync(func)

#endif
