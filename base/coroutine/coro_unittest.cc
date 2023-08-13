#include <stdlib.h>
#include <unistd.h>
#include <atomic>
#include <iostream>

#include "base/utils/rand_util.h"
#include "base/utils/sys_error.h"
#include "fmt/chrono.h"
#include "glog/logging.h"

#include <base/coroutine/co_runner.h>
#include <base/coroutine/wait_group.h>
#include <base/memory/scoped_guard.h>
#include <base/message_loop/message_loop.h>
#include <base/time/time_utils.h>
#include "base/coroutine/co_mutex.h"

#include <catch2/catch_test_macros.hpp>

void coro_c_function() {
  LOG(INFO) << __FUNCTION__ << " enter";
  LOG(INFO) << __FUNCTION__ << " leave";
}

void coro_fun(std::string tag) {
  LOG(INFO) << tag << " coro_fun enter";
  LOG(INFO) << tag << " coro_fun leave";
}

#if 0
TEST_CASE("coro.crash", "[run task as coro crash task]") {
  CO_SLEEP(1000);
}
#endif

TEST_CASE("coro.check", "[run task as coro not working task]") {
  REQUIRE_FALSE(CO_RESUMER);
  REQUIRE_FALSE(CO_CANYIELD);
}

TEST_CASE("coro.with_fmt", "[run task as coro with fmt lib]") {
  FLAGS_v = 26;
  LOG(INFO) << "start test use libfmt in coro";
  CO_GO [&]() {
    auto tm = fmt::gmtime(std::time(nullptr));
    LOG(INFO) << "fmt gm time:" << fmt::format("{:%a, %d %b %Y %H:%M:%S %Z}", tm);
  };
  sleep(1);
  LOG(INFO) << "end test use libfmt in coro";
}

TEST_CASE("coro.background", "[run task as coro task in background]") {
  LOG(INFO) << " start test go flag enter";
  FLAGS_v = 26;
  bool background_ok = false;
  CO_GO [&]() {
    LOG(INFO) << " run lambda in coroutine";
    background_ok = true;
  };
  sleep(1);
  REQUIRE(background_ok);
  LOG(INFO) << " start test go flag leave";
}

TEST_CASE("coro.go", "[go flag call coroutine]") {
  FLAGS_v = 26;
  base::MessageLoop loop;
  loop.Start();
  bool lambda_ok = false;
  LOG(INFO) << " start test go flag enter";

  CO_GO coro_c_function;

  CO_GO std::bind(&coro_fun, "tag_from_go_synatax");

  CO_GO [&]() {
    lambda_ok = true;
    LOG(INFO) << " run lambda in coroutine";
  };
  bool withloop_ok = false;
  CO_GO &loop << [&]() {
    withloop_ok = true;
    LOG(INFO) << "go coroutine with loop ok!!!";
  };

  loop.PostDelayTask(NewClosure([&]() { loop.QuitLoop(); }), 2000);
  loop.WaitLoopEnd();
  REQUIRE(lambda_ok);
  REQUIRE(withloop_ok);
}

TEST_CASE("coro.resumer", "[coroutine resumer with loop reply task]") {
  FLAGS_v = 26;
  base::MessageLoop loop;
  loop.Start();

  bool looptask_ok = false;
  bool gogo_task_ok = false;
  CO_GO [&]() {
    auto loop = base::MessageLoop::Current();
    loop->PostTaskAndReply(FROM_HERE,
                           []() {},
                           CO_RESUMER);
    CO_YIELD;
    looptask_ok = true;
    LOG(INFO) << "first resume back";
    auto resumer = CO_RESUMER;
    CHECK(resumer);
    CO_GO [&]() { resumer(); };
    CO_YIELD;
    LOG(INFO) << "second resume back";
    gogo_task_ok = true;
  };

  loop.PostDelayTask(
      NewClosure([]() { base::MessageLoop::Current()->QuitLoop(); }),
      2000);
  loop.WaitLoopEnd();

  REQUIRE(looptask_ok);
  REQUIRE(gogo_task_ok);
}

TEST_CASE("coro.co_sleep", "[coroutine sleep]") {
  FLAGS_v = 25;

  base::MessageLoop loop;
  loop.Start();

  bool co_sleep_resumed = false;
  CO_GO& loop << [&]() {
    CO_SLEEP(50);
    co_sleep_resumed = true;
  };

  loop.PostDelayTask(NewClosure([&]() { loop.QuitLoop(); }),
                     500);  // exit ater 5s

  loop.WaitLoopEnd();
  REQUIRE(co_sleep_resumed == true);
}

TEST_CASE("coro.multi_resume", "[coroutine resume twice]") {
  FLAGS_v = 25;

  base::MessageLoop loop;
  loop.Start();

  auto stack_sensitive_fn = []() {
    LOG(INFO) << "normal task start...";
    LOG(INFO) << "normal task end...";
  };

  bool resuer_ok = false;

  CO_GO& loop << [&]() {  // main
    LOG(INFO) << "coro run start...";

    loop.PostTask(NewClosure(CO_RESUMER));
    loop.PostDelayTask(NewClosure(CO_RESUMER), 10);
    loop.PostDelayTask(NewClosure(CO_RESUMER), 100);
    loop.PostDelayTask(NewClosure(CO_RESUMER), 200);
    loop.PostTaskAndReply(FROM_HERE, stack_sensitive_fn, CO_RESUMER);

    CO_YIELD;
    resuer_ok = true;
  };

  loop.PostDelayTask(NewClosure([&]() { loop.QuitLoop(); }),
                     500);  // exit ater 5s
  loop.WaitLoopEnd();
  REQUIRE(resuer_ok);
}

TEST_CASE("coro.wg_timeout", "[coroutine resume twice]") {
  FLAGS_v = 25;

  CO_GO[&]() {  // main
    LOG(INFO) << "waig group broadcast coro test start...";

    auto wg = co::WaitGroup::New();

    wg->Add(1);
    CO_GO[wg]() {
      base::ScopedGuard done([wg]() { wg->Done(); });
      CO_SLEEP(500);
    };

    auto res = wg->Wait(10);
    REQUIRE(res == co::WaitGroup::kTimeout);
    LOG(INFO) << "wait group braodcast coro resumed from wait...";
  };

  sleep(1);
}

TEST_CASE("coro.wg_broadcast", "[coroutine braodcast resume]") {
  FLAGS_v = 25;

  CO_GO[&]() {  // main
    LOG(INFO) << "waig group broadcast coro test start...";

    auto wg = co::WaitGroup::New();
    for (int i = 0; i < 2; i++) {
      wg->Add(1);
      CO_GO[wg]() {
        CO_SLEEP(5);
        wg->Done();
      };
    }

    auto res = wg->Wait(1000);
    REQUIRE(res == co::WaitGroup::kSuccess);
    LOG(INFO) << "wait group braodcast resumed.";
  };
  sleep(2);
}

TEST_CASE("coro.co_sync", "[coroutine braodcast resume]") {
  FLAGS_v = 25;

  CO_GO[]() {
    int result = 0;

    CO_SYNC([&]() {
      LOG(INFO) << "sync coro task run...";
      result = 12;
    });

    REQUIRE(result == 12);
  };
  usleep(500);
}

TEST_CASE("coro.Sched", "[coroutine sched]") {
  FLAGS_v = 26;

  int64_t cnt = 0;

  CO_GO[&]() {
    std::cout << "first task run start" << std::endl;
    auto start = base::time_us();
    while (base::time_us() - start < 1000) {
      __co_sched_here__;
    };
    cnt++;
    std::cout << "first task run end" << std::endl;
  };

  CO_GO[&]() {
    cnt++;
    std::cout << "second task run" << std::endl;
  };
  usleep(3000);
  REQUIRE(cnt == 2);
}

TEST_CASE("coro.resume_twice", "[resume_twice a yield coro]") {
  FLAGS_v = 25;

  int64_t cnt = 0;
  base::LtClosure resumer;
  CO_GO[&]() {
    resumer = CO_RESUMER;
    CO_YIELD;
    cnt++;
    CO_YIELD;
    cnt++;
  };
  while(!resumer)
    std::this_thread::yield();

  resumer();
  resumer();
  resumer();
  resumer();

  usleep(5000);
  REQUIRE(cnt == 1);
}

TEST_CASE("coro.co_lock", "[resume_twice a yield coro]") {
  FLAGS_v = 25;

  std::vector<base::MessageLoop*> loops;
  for (int i = 0; i < 10; i++) {
    loops.push_back(new base::MessageLoop(std::to_string(i)));
    loops.back()->Start();
    co::CoroRunner::RegisteRunner(loops.back());
  }

  int64_t cnt = 0;

  co::CoMutex co_lock;
  for (int i = 0; i < 1000; i++) {
    CO_GO [&]() {
      co_lock.lock();
      cnt++;
      co_lock.unlock();
    };
  }
  sleep(1);
  REQUIRE(cnt == 1000);
}

#include <sys/eventfd.h>
#include <base/coroutine/io_event.h>

TEST_CASE("coro.ioevent", "[ioevent for coro]") {
  //FLAGS_v = 26;

  base::MessageLoop loop("main");
  loop.Start();

  int timeout = 20;
  bool running = true;
  int fd = eventfd(0, EFD_NONBLOCK);
  CHECK(fd > 0);

  co_go &loop << [&]() {
    base::FdEvent fdev(fd, base::LtEv::READ);
    co::IOEvent ioev(&fdev);
    do {
      ignore_result(ioev.Wait(timeout));
      uint64_t val = 0;
      if (eventfd_read(fd, &val) == 0) {
        std::cout << ioev.ResultStr() << ", read val:" << val << std::endl;
        continue;
      }
      std::cout << ioev.ResultStr() << ", read err:" << base::StrError() << std::endl;
      if (errno != EAGAIN) {
        break;
      }
    } while(running);
    loop.QuitLoop();
  };

  int cnt = 100;
  while(cnt--) {
    uint64_t v = base::RandInt(timeout / 2, timeout + (timeout / 2));
    usleep(1000 * v); // ms
    ignore_result(eventfd_write(fd, v));
  };
  running = false;
  loop.WaitLoopEnd();
  close(fd);
}

TEST_CASE("coro.ioevent2", "[ioevent for coro]") {
  //FLAGS_v = 26;

  base::MessageLoop loop("main");
  loop.Start();

  bool running = true;
  int fd = eventfd(0, EFD_NONBLOCK);
  CHECK(fd > 0);

  co_go &loop << [&]() {
    base::FdEvent fdev(fd, base::LtEv::READ);
    co::IOEvent ioev(&fdev);
    do {
      ignore_result(ioev.Wait());
      uint64_t val = 0;
      if (eventfd_read(fd, &val) == 0) {
        std::cout << ioev.ResultStr() << ", read val:" << val << std::endl;
        continue;
      }
      std::cout << ioev.ResultStr() << ", read err:" << base::StrError() << std::endl;
      if (errno != EAGAIN) {
        break;
      }
    } while(running);
    loop.QuitLoop();
  };

  int cnt = 100;
  while(cnt--) {
    uint64_t v = base::RandInt(50, 100);
    usleep(1000 * v); // ms
    ignore_result(eventfd_write(fd, v));
  };
  running = false;
  loop.WaitLoopEnd();
  close(fd);
}
