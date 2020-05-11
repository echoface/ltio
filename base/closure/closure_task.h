#ifndef BASE_CLOSURE_TASK_H_H
#define BASE_CLOSURE_TASK_H_H

#include <memory>
#include <assert.h>
#include <functional>
#include <glog/logging.h>
#include <base/base_micro.h>

#include "location.h"

typedef std::function<void()> StlClosure;
typedef std::function<void()> ClosureCallback;

namespace base {

class TaskBase {
public:
  TaskBase() {}
  explicit TaskBase(const Location& location) : location_(location) {}
  virtual ~TaskBase() {}
  virtual void Run() = 0;
  const Location& TaskLocation() const {return location_;}
  std::string ClosureInfo() const {return location_.ToString();}
private:
  Location location_;
};

typedef std::unique_ptr<TaskBase> TaskBasePtr;

template <typename Functor>
class ClosureTask : public TaskBase {
public:
  explicit ClosureTask(const Location& location, const Functor& closure)
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
                  const Closure& closure,
                  const Cleanup& cleanup)
    : TaskBase(location),
      closure_task(closure),
      cleanup_task(cleanup) {
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


template <class Closure>
static TaskBasePtr CreateClosure(const Location& location,
                                 const Closure& closure) {
  return TaskBasePtr(new ClosureTask<Closure>(location, closure));
}

template <class Closure, class Cleanup>
static TaskBasePtr CreateTaskWithCallback(const Location& location,
                                          const Closure& closure,
                                          const Cleanup& cleanup) {
  return TaskBasePtr(new TaskWithCleanup<Closure,Cleanup>(location, closure, cleanup));
}

}// end base namespace

#define NewClosure(Functor) ::base::CreateClosure(FROM_HERE, Functor)
#define NewClosureWithCleanup(Functor, Cleanup) ::base::CreateClosureWithCallback(Location, Functor, Cleanup)


#endif
