#ifndef BASE_TimeoutEvent_EVENT_H_
#define BASE_TimeoutEvent_EVENT_H_

#include <cinttypes>
#include "time/timestamp.h"
#include "closure/closure_task.h"
#include <thirdparty/timeout/timeout.h>

typedef struct timeout Timeout;
namespace base {

class TimeoutEvent : public Timeout {
public:
  static TimeoutEvent* CreateOneShot(uint64_t ms, bool delelte_after_invoke);

  TimeoutEvent(uint64_t ms, bool repeat);
  ~TimeoutEvent();

  void Invoke();
  void UpdateInterval(int64_t ms);
  void InstallTimerHandler(TaskBasePtr&& h);
  bool IsRepeated() const {return repeat_;}
  inline bool DelAfterInvoke() const {return del_after_invoke_;}
  inline bool IsAttached() const {return pending != NULL;}
  inline uint64_t Interval() const {return interval_;}
  inline uint64_t IntervalMicroSecond() const {return interval_ * 1000;}
private:
  bool repeat_ = false;
  bool use_coro_ = false;
  uint64_t interval_ = 0; //ms
  bool del_after_invoke_ = false;
  TaskBasePtr timer_handler_;
};

}
#endif
