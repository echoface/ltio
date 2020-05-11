#include <unistd.h>
#include <iostream>
#include <unistd.h>
#include <stdlib.h>
#include <atomic>

#include "glog/logging.h"
#include <base/time/time_utils.h>
#include <base/coroutine/wait_group.h>
#include <base/message_loop/message_loop.h>
#include <base/coroutine/coroutine_runner.h>

#include <thirdparty/catch/catch.hpp>

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

TEST_CASE("coro.co_resumer()", "[coroutine resumer with loop reply task]") {

  base::MessageLoop loop;
  loop.Start();

  StlClosure stack_sensitive_fn = []() {
    sleep(1);
  };

  bool stack_sensitive_fn_resumed = false;

  // big stack function;
  co_go &loop << [&]() {
    LOG(INFO) << " coroutine enter ..";
    loop.PostTaskAndReply(FROM_HERE, stack_sensitive_fn, co_resumer());
    LOG(INFO) << " coroutine pasued..";
    co_pause;
    LOG(INFO) << " coroutine resumed..";
    stack_sensitive_fn_resumed = true;

    loop.QuitLoop();
  };


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
  }), 5000); // exit ater 5s

  loop.WaitLoopEnd();

  REQUIRE(co_sleep_resumed == true);
}

TEST_CASE("coro.waitGroup", "[coroutine resumer with loop reply task]") {
  base::MessageLoop loop; loop.Start();

  co_go &loop << [&]() { //main
    LOG(INFO) << "main start...";

    base::WaitGroup wg;

    co_go [&]() {
      wg.Add(1);
      co_sleep(500);
      wg.Done();
    };

    co_go [&]() {
      wg.Add(1);
      co_sleep(501);
      wg.Done();
    };

    co_go [&]() {
      wg.Add(1);
      co_sleep(490);
      wg.Done();
    };

    wg.Wait(500);
    loop.QuitLoop();
  };
  loop.WaitLoopEnd();
}

TEST_CASE("coro.multi_resume", "[coroutine resume twice]") {
  base::MessageLoop loop; loop.Start();

  auto stack_sensitive_fn = []() {
    LOG(INFO) << "normal task start...";
    sleep(1);
    LOG(INFO) << "normal task end...";
  };

  co_go &loop << [&]() { //main
    LOG(INFO) << "coro run start...";

    loop.PostDelayTask(NewClosure(co_resumer()), 201);
    loop.PostDelayTask(NewClosure(co_resumer()), 200);
    loop.PostTaskAndReply(FROM_HERE, stack_sensitive_fn, co_resumer());

    LOG(INFO) << "coro run resumed...";

    loop.QuitLoop();
  };
  loop.WaitLoopEnd();
}


