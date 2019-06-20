#ifndef BASE_CLOSURE_TASK_H_H
#define BASE_CLOSURE_TASK_H_H

#include <list>
#include <memory>
#include <queue>
#include <assert.h>
#include <functional>
#include "glog/logging.h"
#include "location.h"
#include "base/base_micro.h"
#include <atomic>

#include <concurrentqueue/concurrentqueue.h>

#define NewClosureAlias(Location, Functor) ::base::CreateClosure(Location, Functor)
#define NewClosureWithCallbackAlias(Location, Functor, Cleanup) ::base::CreateClosureWithCallback(Location, Functor, Cleanup)

#define NewClosure(Functor) NewClosureAlias(FROM_HERE, Functor)
#define NewClosureWithCleanup(Functor, Cleanup) NewClosureWithCallbackAlias(Location, Functor, Cleanup)

namespace base {

typedef std::function<void()> StlClosure;

class TaskBase {
public:
  TaskBase() {}
  explicit TaskBase(const Location& location) : location_(location) {}
  virtual ~TaskBase() {}
  virtual void Run() = 0;
  void operator()() {
    return Run();
  }
  const Location& TaskLocation() const {return location_;}
  std::string ClosureInfo() const {return location_.ToString();}
private:
  Location location_;
};

typedef std::unique_ptr<TaskBase> TaskBasePtr;

template <typename Functor>
class ClosureTask : public TaskBase {
public:
  explicit ClosureTask(const Location& location, const Functor closure)
   : TaskBase(location),
     closure_task(closure) {
  }
  void Run() override {
    try {
      closure_task();
    } catch (...) {
      LOG(ERROR) << "Task Crash, From:" << ClosureInfo();
      abort();
    }
  }
private:
  Functor closure_task;
};

template <class Closure, class Cleanup>
class TaskWithCleanup : public TaskBase {
public:
  TaskWithCleanup(const Location& location,
                  Closure& closure,
                  Cleanup& cleanup)
    : TaskBase(location),
      closure_task(std::move(closure)),
      cleanup_task(std::move(cleanup)) {
  }
  void Run() override {
    try {
      closure_task();
    } catch (...) {
      LOG(ERROR) << "Task Crash, From:" << ClosureInfo();
      abort();
    }
    try {
      cleanup_task();
    } catch (...) {
      LOG(ERROR) << "Cleanup Crash, From:" << ClosureInfo();
      abort();
    }
  }
private:
  Closure closure_task;
  Cleanup cleanup_task;
};

template <>
class TaskWithCleanup<TaskBasePtr, TaskBasePtr> : public TaskBase {
public:
  TaskWithCleanup(const Location& location,
                  TaskBasePtr& closure,
                  TaskBasePtr& cleanup)
    : TaskBase(location),
      closure_task(std::move(closure)),
      cleanup_task(std::move(cleanup)) {
  }
  void Run() override {
    closure_task->Run();
    cleanup_task->Run();
  }
private:
  TaskBasePtr closure_task;
  TaskBasePtr cleanup_task;
};


template <class Closure>
static TaskBasePtr CreateClosure(const Location& location,
                                 const Closure& closure) {
  return TaskBasePtr(new ClosureTask<Closure>(location, closure));
}

template <class Closure, class Cleanup>
static TaskBasePtr CreateTaskWithCallback(const Location& location,
                                          Closure& closure,
                                          Cleanup& cleanup) {
  return TaskBasePtr(new TaskWithCleanup<Closure,Cleanup>(location, closure, cleanup));
}

}// end namespace
#endif
