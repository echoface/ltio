#ifndef BASE_REPEATING_TIMER_H_H
#define BASE_REPEATING_TIMER_H_H

#include <memory>
#include <inttypes.h>
#include <base/base_micro.h>
#include <base/closure/closure_task.h>
#include <base/message_loop/message_loop.h>
#include <atomic>

#include <mutex>
#include <condition_variable>

namespace base {
// Sample RepeatingTimer usage:
//
//   class MyClass {
//    public:
//     void StartDoingStuff() {
//       timer_.Start(delay_ms, ClosureTask);
//     }
//     void StopStuff() {
//       timer_.Stop();
//     }
//    private:
//     void DoStuff() {
//       // This method is called every second to do stuff.
//       ...
//     }
//     base::RepeatingTimer timer_;
//   };
//
class MessageLoop;

/*start and stop are not thread safe*/
class RepeatingTimer {
public:
  // new a timer with spcial runner loop
  RepeatingTimer(MessageLoop* loop);
  ~RepeatingTimer();

  void Start(uint64_t ms, StlClosure user_task);
  // a sync stop
  void Stop();

  bool Running() const;
private:
  void OnTimeout();

  bool running_;
  StlClosure user_task_;
  MessageLoop* runner_loop_ = NULL;

  std::unique_ptr<TimeoutEvent> timeout_event_;

  std::mutex m;
  std::condition_variable cv;
  DISALLOW_COPY_AND_ASSIGN(RepeatingTimer);
};

}
#endif
