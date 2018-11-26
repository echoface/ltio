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
  static TimeoutEvent* CreateSelfDeleteTimeoutEvent(uint64_t ms);

  TimeoutEvent(uint64_t ms, bool repeat);
  ~TimeoutEvent();

  void InvokeTimerHanlder();
  void UpdateInterval(int64_t ms);
  void InstallTimerHandler(ClosurePtr&& h);
  bool IsRepeated() const {return repeat_;}
  bool SelfDelete() const {return self_delete_;}
  uint64_t Interval() const {return interval_;}
  uint64_t IntervalMicroSecond() const {return interval_ * 1000;}
  bool IsAtatched() const {return this->pending != NULL;}
private:
  bool repeat_ = false;
  uint64_t interval_ = 0;
  bool self_delete_ = false;
  ClosurePtr timer_handler_;
};

}
#endif
