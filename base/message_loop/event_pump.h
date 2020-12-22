#ifndef BASE_EVENT_PUMP_H_
#define BASE_EVENT_PUMP_H_

#include <inttypes.h>

#include <map>
#include <memory>
#include <thread>
#include <vector>

#include "fd_event.h"
#include "io_multiplexer.h"
#include "timeout_event.h"
#include <thirdparty/timeout/timeout.h>

namespace base {

class IOMux;

typedef struct timeouts TimeoutWheel;
typedef std::unique_ptr<TimeoutWheel> TimeoutWheelPtr;

typedef std::function<void()> QuitClosure;
typedef std::shared_ptr<FdEvent> RefFdEvent;
typedef std::vector<TimeoutEvent*> TimerEventList;

class PumpDelegate {
public:
  virtual void PumpStarted() {};
  virtual void PumpStopped() {};
  virtual void RunNestedTask() {};
  virtual uint64_t PumpTimeout() {return 5;}; // ms
  virtual void RunTimerClosure(const TimerEventList&) {};
};

/* pump fd event and timeout event and pass them to handler by delegate interface*/
class EventPump {
public:
  EventPump();
  EventPump(PumpDelegate* d);
  ~EventPump();

  void Run();

  void Quit() {running_ = false;};

  bool InstallFdEvent(FdEvent *fd_event);

  bool RemoveFdEvent(FdEvent* fd_event);

  void AddTimeoutEvent(TimeoutEvent* timeout_ev);

  void RemoveTimeoutEvent(TimeoutEvent* timeout_ev);

  bool IsInLoopThread() const;

  bool Running() { return running_; }

  void SetLoopThreadId(std::thread::id id) {tid_ = id;}
protected:
  /* update the time wheel mononic time and get all expired
   * timeoutevent will be invoke and re shedule if was repeated*/
  void ProcessTimerEvent();

  void InvokeFiredEvent(FiredEvent* start, int count);
  /* return the minimal timeout for next poll
   * return 0 when has event expired,
   * return default_timeout when no other timeout provided*/
  timeout_t NextTimeout();

  /* finalize TimeoutWheel and delete Life-Ownered timeoutEvent*/
  void InitializeTimeWheel();
  void FinalizeTimeWheel();

  /*calculate abs timeout time and add to timeout wheel*/
  void add_timer_internal(uint64_t now_us, TimeoutEvent* event);
private:

  PumpDelegate* delegate_;
  bool running_;

  std::unique_ptr<IOMux> io_mux_;

#if 0
  // replace by new timeout_wheel for more effective perfomance [ O(1) ]
  //TimerTaskQueue timer_queue_;
#endif
  int32_t max_fds_;
  std::thread::id tid_;
  int32_t timestamp_;
  TimeoutWheel* timeout_wheel_ = nullptr;
};

}
#endif
