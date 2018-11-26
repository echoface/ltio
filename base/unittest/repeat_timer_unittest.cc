#include <catch/catch.hpp>

#include <message_loop/message_loop.h>
#include <message_loop/repeating_timer.h>

TEST_CASE("repeat_timer.normal", "[repeating time test]") {
  base::MessageLoop loop;
  loop.Start();

  int64_t invoke_count = 0;
  int64_t interval_ms = 5;
  int64_t total_time_ms = 30000;

  base::RepeatingTimer timer(&loop);
  timer.Start(interval_ms, [&]() {
    invoke_count++;
  });

  loop.PostDelayTask(NewClosure([&]() {
    loop.QuitLoop();
  }), total_time_ms);

  loop.WaitLoopEnd();
  LOG(INFO) << "should finish:" << total_time_ms/interval_ms << " finished:" << invoke_count;
  REQUIRE((invoke_count == total_time_ms/interval_ms));
}
