#ifndef BASE_EVENT_PUMP_H_
#define BASE_EVENT_PUMP_H_

#include <vector>
#include <memory>
#include <map>
#include <inttypes.h>
#include <thread>

#include "fd_event.h"
#include "timer_task_queue.h"
#include "timeout_event.h"
#include <thirdparty/timeout/timeout.h>

namespace base {

class IoMultiplexer;

typedef struct timeouts TimeoutWheel;
typedef std::function<void()> QuitClourse;
typedef std::shared_ptr<FdEvent> RefFdEvent;

class PumpDelegate {
public:
  virtual void RunNestedTask() {};
  virtual void BeforePumpRun() {};
  virtual void AfterPumpRun() {};
};

/* pump fd event and timeout event*/
class EventPump : public FdEvent::FdEventWatcher {
public:
  EventPump();
  EventPump(PumpDelegate* d);
  ~EventPump();

  void Run();
  void Quit();

  bool InstallFdEvent(FdEvent *fd_event);
  bool RemoveFdEvent(FdEvent* fd_event);

  // override from FdEventWatcher
  void OnEventChanged(FdEvent* fd_event) override;

  bool CancelTimer(uint32_t timer_id);
  int32_t ScheduleTimer(RefTimerEvent& timerevent);

  void AddTimeoutEvent(TimeoutEvent* timeout_ev);
  void RemoveTimeoutEvent(TimeoutEvent* timeout_ev);

  QuitClourse Quit_Clourse();
  bool IsInLoopThread() const;

  bool Running() { return running_; }
  void SetLoopThreadId(std::thread::id id) {tid_ = id;}
  uint64_t NonePreciseMillsecond() {return timer_queue_.InexacTimeMillsecond();}
protected:
  inline FdEvent::FdEventWatcher* AsFdWatcher() {return this;}

  /* update the time wheel mononic time and get all expired
   * timeoutevent will be invoke and re shedule if was repeated*/
  void ProcessTimerEvent();

  /* return the possible minimal wait-time for next timer
   * return 0 when has event expired,
   * return default_timeout when no timer*/
  timeout_t NextTimerTimeoutms(timeout_t default_timeout);

  /*calculate abs timeout time and add to timeout wheel*/
  void add_timer_internal(TimeoutEvent* event);
private:
  PumpDelegate* delegate_;
  bool running_;

  std::vector<FdEvent*> active_events_;
  std::unique_ptr<IoMultiplexer> multiplexer_;

  TimeoutWheel* timeout_wheel_ = NULL;

  TimerTaskQueue timer_queue_;
  std::thread::id tid_;
};

}
#endif
