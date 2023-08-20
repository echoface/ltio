#include <catch2/catch_test_macros.hpp>

#include "base/closure/closure_task.h"
#include "glog/logging.h"

#include <base/coroutine/co_runner.h>
#include <base/message_loop/event_pump.h>
#include <base/message_loop/message_loop.h>
#include <iostream>

class Stub {
public:
  void func(int a, int b) { LOG(INFO) << __FUNCTION__ << " invoked "; };
};

Stub stub;

void FailureDump(const char* s, int sz) {
  std::string failure_info(s, sz);
  LOG(INFO) << " ERROR: " << failure_info;
}

void cfunc() {
  LOG(INFO) << __func__ << " c function run";
}

class TaskMock {
public:
  void member_fun() { LOG(INFO) << __func__ << " run"; }
  void member_fun_args(int v) { LOG(INFO) << __func__ << " run, arg:" << v; }
  static void static_member_fun() { LOG(INFO) << __func__ << " run, arg:"; }
};

CATCH_TEST_CASE("loop.benchStart", "[test event start]") {
  for (int i = 0; i < 10; i++) {
    base::MessageLoop loop;
    loop.SetLoopName("ut_" + std::to_string(i));
    loop.Start();

    loop.PostTask(FROM_HERE, &base::MessageLoop::QuitLoop, &loop);

    loop.WaitLoopEnd();
    break;
  }
  sleep(1);
}

CATCH_TEST_CASE("base.task", "[test event pump timer]") {
  auto f = NewClosure(&cfunc);
  f->Run();
  auto lambda_task = NewClosure([]() { ; });
  lambda_task->Run();

  TaskMock mock;
  auto static_member_fun_task = NewClosure(&TaskMock::static_member_fun);
  static_member_fun_task->Run();

  auto member_fun_task = NewClosure(std::bind(&TaskMock::member_fun, &mock));
  member_fun_task->Run();
}

CATCH_TEST_CASE("event_pump.timer", "[test event pump timer]") {
  base::EventPump pump;
  pump.SetLoopId(base::MessageLoop::GenLoopID());
  pump.PrepareRun();

  size_t repeated_times = 0;
  bool oneshot_invoked = false;

  base::TimeoutEvent* repeated_toe = new base::TimeoutEvent(5, true);
  repeated_toe->InstallHandler(NewClosure([&]() { repeated_times++; }));

  auto start = base::time_ms();
  std::cout << "start at ms:" << start << std::endl;

  bool running = true;

  base::TimeoutEvent* quit_toe = base::TimeoutEvent::CreateOneShot(1000, false);
  quit_toe->InstallHandler(NewClosure([&]() {
    auto end = base::time_ms();
    std::cout << "stop at ms:" << end - start << std::endl;
    CATCH_REQUIRE(end - start >= 1000);

    oneshot_invoked = true;
    pump.RemoveTimeoutEvent(quit_toe);
    pump.RemoveTimeoutEvent(repeated_toe);

    running = false;
  }));

  pump.AddTimeoutEvent(quit_toe);
  pump.AddTimeoutEvent(repeated_toe);

  while(running) {
    pump.Pump(0);
  }
  std::cout << "repeated_times:" << repeated_times << std::endl;

  CATCH_REQUIRE(oneshot_invoked);
  CATCH_REQUIRE(repeated_times > 190);

  delete quit_toe;
  delete repeated_toe;
}

CATCH_TEST_CASE("messageloop.delaytask", "[run delay task]") {
  base::MessageLoop loop;
  loop.SetLoopName("DelayTaskTestLoop");
  loop.Start();

  uint64_t start = base::time_ms();
  loop.PostDelayTask(NewClosure([&]() {
                       uint64_t end = base::time_ms();
                       LOG(INFO) << "delay task run:" << end - start
                                 << "(ms), expect 500";
                       CATCH_REQUIRE(((end - start >= 500) && (end - start <= 550)));
                     }),
                     500);

  loop.PostDelayTask(NewClosure([&]() { loop.QuitLoop(); }), 1000);

  loop.WaitLoopEnd();
}

CATCH_TEST_CASE("messageloop.replytask", "[task with reply function]") {
  base::MessageLoop loop;
  loop.Start();

  base::MessageLoop replyloop;
  replyloop.Start();

  int64_t replytask_invoked_times = 0;

  loop.PostTaskAndReply(FROM_HERE,
                        [&]() { LOG(INFO) << " task bind reply in loop run"; },
                        [&]() {
                          replytask_invoked_times++;
                          CATCH_REQUIRE(base::MessageLoop::Current() == &loop);
                        });

  loop.PostTaskAndReply(
      FROM_HERE,
      [&]() { LOG(INFO) << " task bind reply use another loop run"; },
      [&]() {
        replytask_invoked_times++;
        CATCH_REQUIRE(base::MessageLoop::Current() == &replyloop);
      },
      &replyloop);

  loop.PostDelayTask(NewClosure([&]() {
                       loop.QuitLoop();
                       replyloop.QuitLoop();
                     }),
                     100);
  loop.WaitLoopEnd();
  CATCH_REQUIRE(replytask_invoked_times == 2);
}

int64_t start_time;
static std::int64_t counter = 0;
static const std::int64_t kTaskCount = 10000000;

enum BenchMode {
  LOOP,
  CORO,
  CORO_WITH_LOOP,
};

void invoke(base::MessageLoop* loop, BenchMode mode) {
  if (counter++ == kTaskCount) {
    int64_t diff = base::time_us() - start_time;
    int64_t tps = kTaskCount * counter / diff;
    LOG(INFO) << "one by one bench, mode:" << mode
              << ", total:" << diff << ", " << tps << "/sec";
    loop->QuitLoop();
    return;
  };

  switch(mode) {
    case LOOP:
      loop->PostTask(FROM_HERE, invoke, loop, mode);
      break;
    case CORO:
      CO_GO std::bind(invoke, loop, mode);
      break;
    case CORO_WITH_LOOP:
      CO_GO loop << std::bind(invoke, loop, mode);
      break;
    default:
      break;
  }
}

CATCH_TEST_CASE("co_task_obo_bench",
          "[schedule coro task one after one]") {
  FLAGS_v = 0;
  base::MessageLoop loop;
  loop.Start();

  LOG(INFO) << __FUNCTION__ << ", co_task_obo_bench run";
  counter = 0;
  CO_GO &loop << std::bind(invoke, &loop, BenchMode::CORO);
  start_time = base::time_us();
  LOG(INFO) << __FUNCTION__ << ", wait loop end:" << loop.LoopName();
  loop.WaitLoopEnd();
  LOG(INFO) << __FUNCTION__ << ", co_task_obo_bench end";
}

CATCH_TEST_CASE("co_task_obo_bench_bind_loop",
          "[schedule coro task one after one with specific loop]") {

  FLAGS_v = 0;
  base::MessageLoop loop;
  loop.Start();

  LOG(INFO) << __FUNCTION__ << ", co_task_obo_bench_bind_loop run";
  counter = 0;
  loop.PostTask(FROM_HERE, invoke, &loop, BenchMode::CORO_WITH_LOOP);
  start_time = base::time_us();

  LOG(INFO) << __FUNCTION__ << ", wait loop end:" << loop.LoopName();
  loop.WaitLoopEnd();
  LOG(INFO) << __FUNCTION__ << ", co_task_obo_bench_bind_loop end";
}

CATCH_TEST_CASE("task_obo_bench",
          "[schedule normal task one after one]") {
  FLAGS_v = 0;
  base::MessageLoop loop;
  loop.Start();

  LOG(INFO) << __FUNCTION__ << ", task_obo_bench run";
  counter = 0;
  loop.PostTask(FROM_HERE, invoke, &loop, BenchMode::LOOP);
  start_time = base::time_us();

  LOG(INFO) << __FUNCTION__ << ", wait loop end:" << loop.LoopName();
  loop.WaitLoopEnd();
  LOG(INFO) << __FUNCTION__ << ", task_obo_bench end";
}
