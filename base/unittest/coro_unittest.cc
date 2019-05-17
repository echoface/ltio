#include <unistd.h>
#include <iostream>
#include <unistd.h>
#include <stdlib.h>
#include <atomic>

#include "glog/logging.h"
#include <time/time_utils.h>
#include <coroutine/coroutine.h>
#include <message_loop/message_loop.h>
#include <coroutine/coroutine_runner.h>
#include <coroutine/wait_group.h>

#include <catch/catch.hpp>

void coro_c_function() {
  LOG(INFO) << __FUNCTION__ << " enter";
  LOG(INFO) << __FUNCTION__ << " leave";
}

void coro_fun(std::string tag) {
  LOG(INFO) << tag << " coro_fun enter";
  LOG(INFO) << tag << " coro_fun leave";
}

TEST_CASE("coro.co_go", "[go flag call coroutine]") {
  base::MessageLoop loop;
  loop.Start();

  loop.PostTask(NewClosure([&]() {
    LOG(INFO) << " start test go flag enter";

    co_go coro_c_function;

    co_go std::bind(&coro_fun, "tag_from_go_synatax");

    co_go []() {
      LOG(INFO) << " run lambda in coroutine" ;
    };
    LOG(INFO) << " start test go flag leave";
  }));

  co_go &loop << []() {
    LOG(INFO) << "go coroutine in loop ok!!!";
  };

  loop.PostDelayTask(NewClosure([&](){
    loop.QuitLoop();
  }), 2000);
  loop.WaitLoopEnd();
}

TEST_CASE("coro.co_resumer", "[coroutine resumer with loop reply task]") {

  base::MessageLoop loop;
  loop.Start();

  base::StlClosure stack_sensitive_fn = []() {
    sleep(1);
  };

  bool stack_sensitive_fn_resumed = false;

  // big stack function;
  co_go &loop << [&]() {
    LOG(INFO) << " coroutine enter ..";
    loop.PostTaskAndReply(NewClosure(stack_sensitive_fn), NewClosure(co_resumer));
    LOG(INFO) << " coroutine pasued..";
    co_yield;
    LOG(INFO) << " coroutine resumed..";
    stack_sensitive_fn_resumed = true;
  };

  loop.PostDelayTask(NewClosure([&]() {
    loop.QuitLoop();
  }), 5000); // exit ater 5s

  loop.WaitLoopEnd();

  REQUIRE(stack_sensitive_fn_resumed);
}

TEST_CASE("coro.co_sleep", "[coroutine sleep]") {

  base::MessageLoop loop;
  loop.Start();

  bool co_sleep_resumed = false;

  // big stack function;
  co_go &loop << [&]() {
    LOG(INFO) << " coroutine enter ..";
    LOG(INFO) << " coroutine pasued..";
    co_sleep(50);
    LOG(INFO) << " coroutine resumed..";
    co_sleep_resumed = true;
  };

  loop.PostDelayTask(NewClosure([&]() {
    loop.QuitLoop();
  }), 1000); // exit ater 5s

  loop.WaitLoopEnd();

  REQUIRE(co_sleep_resumed);
}

TEST_CASE("coro.waitGroup", "[coroutine resumer with loop reply task]") {
  base::MessageLoop loop; loop.Start();

  co_go &loop << [&]() { //main
    LOG(INFO) << "main start...";

    base::WaitGroup wg;

    co_go [&]() {
      wg.Add(1); LOG(INFO) << "task 1 run...";

      co_sleep(100);

      wg.Done(); LOG(INFO) << "task 1 done...";
    };

    co_go [&]() {
      wg.Add(1); LOG(INFO) << "task 2 run...";

      co_sleep(200);

      wg.Done(); LOG(INFO) << "task 2 done...";
    };

    co_go [&]() {
      wg.Add(1); LOG(INFO) << "task 3 run...";

      co_sleep(150);

      wg.Done(); LOG(INFO) << "task 3 done...";
    };

    wg.Wait(50);
    loop.QuitLoop();
  };
  loop.WaitLoopEnd();
}

