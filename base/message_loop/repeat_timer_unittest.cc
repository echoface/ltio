#include <catch2/catch_test_macros.hpp>

#undef CHECK
#include <base/message_loop/message_loop.h>
#include <base/message_loop/repeating_timer.h>

CATCH_TEST_CASE("repeat_timer.normal", "[repeating time test]") {
  base::MessageLoop loop;
  loop.Start();

  int64_t invoke_count = 0;
  int64_t interval_ms = 5;
  int64_t total_time_ms = 30000;

  base::RepeatingTimer timer(&loop);
  timer.Start(interval_ms, [&]() { invoke_count++; });

  loop.PostDelayTask(NewClosure([&]() {
                       timer.Stop();
                       loop.QuitLoop();
                     }),
                     total_time_ms + 300);

  loop.WaitLoopEnd();
  LOG(INFO) << "expect:" << total_time_ms / interval_ms
            << " actually:" << invoke_count;
}

CATCH_TEST_CASE("repeat_timer.stop", "[repeating time stop test]") {
  base::MessageLoop loop;
  loop.Start();

  int64_t invoke_count = 0;
  int64_t interval_ms = 5;
  int64_t total_time_ms = 5000;

  loop.PostDelayTask(NewClosure([&]() { loop.QuitLoop(); }), total_time_ms);

  base::RepeatingTimer timer(&loop);
  timer.Start(interval_ms, [&]() { invoke_count++; });

  LOG(INFO) << " test stop and start again out of loop call stop<<<<< ";
  sleep(1);
  timer.Stop();
  LOG(INFO) << " timer stoped...";
  bool restart_run = false;
  timer.Start(10, [&]() { restart_run = true; });

  sleep(1);
  LOG(INFO) << " test stop and start again by timer.start call <<<<< ";
  timer.Start(50, [&]() { restart_run = true; });

  sleep(1);
  timer.Stop();

  LOG(INFO) << " test stop in timer handler call timer.stop <<<<< ";
  int stop_in_timer_handler_run = 0;
  timer.Start(50, [&]() {
    stop_in_timer_handler_run++;
    timer.Stop();
  });

  loop.WaitLoopEnd();

  CATCH_REQUIRE(restart_run);
  CATCH_REQUIRE(stop_in_timer_handler_run == 1);
}
