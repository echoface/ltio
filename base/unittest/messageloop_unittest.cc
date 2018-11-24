#include <catch/catch.hpp>

#include <iostream>
#include "message_loop/event_pump.h"

TEST_CASE("event_pump.timer", "[test event pump timer]") {
  
  base::EventPump pump;
  pump.SetLoopThreadId(std::this_thread::get_id());


  uint64_t repeated_timer_checker = 0;
  base::TimeoutEvent* repeated_toe = new base::TimeoutEvent(5000, true);
  repeated_toe->InstallTimerHandler([&]() {
    uint64_t t = base::time_ms();
    uint64_t diff = t - repeated_timer_checker;
    REQUIRE(((diff >= 5000) && (diff <= 5001)));
    repeated_timer_checker = t;
  });

  uint64_t timer_ms_checker = 0;
  base::TimeoutEvent* oneshot_toe = new base::TimeoutEvent(5, false);
  oneshot_toe->InstallTimerHandler([&]() {
    uint64_t t = base::time_ms();
    uint64_t diff = t - timer_ms_checker;
    REQUIRE((diff >= 5 && diff <= 6));
  });

  uint64_t timer_zero_checker = 0;
  base::TimeoutEvent* zerooneshot_toe = new base::TimeoutEvent(0, false);
  zerooneshot_toe->InstallTimerHandler([&]() {
    uint64_t t = base::time_ms();
    uint64_t diff = t - timer_zero_checker;
    REQUIRE((diff >= 0 && diff <= 1));
  });

  uint64_t start, end;
  base::TimeoutEvent* quit_toe = base::TimeoutEvent::CreateSelfDeleteTimeoutEvent(30000); 
  quit_toe->InstallTimerHandler([&]() {
    end = base::time_ms();
    std::cout << "end at ms:" << end << std::endl;
    REQUIRE(end - start == 30000);
    pump.RemoveTimeoutEvent(repeated_toe); 
    pump.RemoveTimeoutEvent(oneshot_toe); 
    pump.RemoveTimeoutEvent(zerooneshot_toe); 
    quit_toe = nullptr;
    pump.Quit();
  });
  
  repeated_timer_checker = timer_ms_checker = timer_zero_checker = start = base::time_ms();
  std::cout << "start at ms:" <<  timer_ms_checker << std::endl;
  pump.AddTimeoutEvent(oneshot_toe);
  pump.AddTimeoutEvent(repeated_toe);
  pump.AddTimeoutEvent(zerooneshot_toe);
  pump.AddTimeoutEvent(quit_toe);

  pump.Run();
}
