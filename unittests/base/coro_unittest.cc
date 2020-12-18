#include <unistd.h>
#include <iostream>
#include <unistd.h>
#include <stdlib.h>
#include <atomic>

#include "glog/logging.h"

#include <base/coroutine/coroutine_runner.h>
#include <base/coroutine/wait_group.h>
#include <base/message_loop/message_loop.h>
#include <base/memory/scoped_guard.h>
#include <base/time/time_utils.h>

#include <thirdparty/catch/catch.hpp>

void coro_c_function() {
  LOG(INFO) << __FUNCTION__ << " enter";
  LOG(INFO) << __FUNCTION__ << " leave";
}

void coro_fun(std::string tag) {
  LOG(INFO) << tag << " coro_fun enter";
  LOG(INFO) << tag << " coro_fun leave";
}

TEST_CASE("coro.crash", "[run task as coro crash task]") {
  // CO_YIELD;

  CO_SLEEP(1000);

  REQUIRE_FALSE(CO_RESUMER);
  REQUIRE_FALSE(CO_CANYIELD);
}

TEST_CASE("coro.background", "[run task as coro task in background]") {
  LOG(INFO) << " start test go flag enter";
  CO_GO coro_c_function;
  CO_GO std::bind(&coro_fun, "tag_from_go_synatax");
  CO_GO []() {
    LOG(INFO) << " run lambda in coroutine" ;
  };
  LOG(INFO) << " start test go flag leave";
}

TEST_CASE("coro.CO_GO", "[go flag call coroutine]") {
  base::MessageLoop loop;
  loop.Start();

  LOG(INFO) << " start test go flag enter";

  CO_GO coro_c_function;

  CO_GO std::bind(&coro_fun, "tag_from_go_synatax");

  CO_GO []() {
    LOG(INFO) << " run lambda in coroutine" ;
  };
  LOG(INFO) << " start test go flag leave";

  CO_GO &loop << []() {
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

  base::LtClosure stack_sensitive_fn = []() {
    LOG(INFO) << " normal task run enter..";
    sleep(1);
    LOG(INFO) << " normal task run and leave..";
  };

  bool stack_sensitive_fn_resumed = false;

  // big stack function;
  CO_GO &loop << [&]() {
    LOG(INFO) << " coroutine enter post a reply task to resume coro..";

    loop.PostTaskAndReply(FROM_HERE, stack_sensitive_fn, CO_RESUMER);

    LOG(INFO) << " coroutine pasued..";

    CO_YIELD;

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
  CO_GO &loop << [&]() {
    LOG(INFO) << " coroutine enter ..";
    LOG(INFO) << " coroutine pasued..";
    CO_SLEEP(50);
    LOG(INFO) << " coroutine resumed..";
    co_sleep_resumed = true;
  };

  loop.PostDelayTask(NewClosure([&]() {
    loop.QuitLoop();
  }), 5000); // exit ater 5s

  loop.WaitLoopEnd();

  REQUIRE(co_sleep_resumed == true);
}

TEST_CASE("coro.multi_resume", "[coroutine resume twice]") {
  base::MessageLoop loop; loop.Start();

  auto stack_sensitive_fn = []() {
    LOG(INFO) << "normal task start...";
    sleep(1);
    LOG(INFO) << "normal task end...";
  };

  CO_GO &loop << [&]() { //main
    LOG(INFO) << "coro run start...";

    loop.PostDelayTask(NewClosure(CO_RESUMER), 201);
    loop.PostDelayTask(NewClosure(CO_RESUMER), 200);
    loop.PostTaskAndReply(FROM_HERE, stack_sensitive_fn, CO_RESUMER);

    LOG(INFO) << "coro run resumed...";

    loop.QuitLoop();
  };
  loop.WaitLoopEnd();
}

TEST_CASE("coro.broadcast", "[coroutine resume twice]") {
  base::MessageLoop loop, loop2;

  loop.SetLoopName("l1"); loop.Start();
  base::CoroRunner::RegisteAsCoroWorker(&loop);

  loop2.SetLoopName("l2"); loop2.Start();
  base::CoroRunner::RegisteAsCoroWorker(&loop2);

  CO_GO [&]() { //main
    LOG(INFO) << "waig group broadcast coro test start...";
    base::WaitGroup wg;

    wg.Add(1);
    loop.PostDelayTask(NewClosure([&wg](){
      LOG(INFO) << "deplay task run";
      wg.Done();
    }), 200);

    for (int i = 0; i < 10; i++) {
      wg.Add(1);

      CO_GO [&wg]() {
        base::ScopedGuard([&wg]() {wg.Done();});

        base::MessageLoop* l = base::MessageLoop::Current();

        LOG(INFO) << "normal task start..." << l->LoopName();
        CO_SLEEP(5);
        // mock network stuff
        // request = BuildHttpRequest();
        // auto response = client.Get(request, {})
        // handle_response();
        LOG(INFO) << "normal task end..." << l->LoopName();
      };
    }

    wg.Wait();

    loop.QuitLoop();
    LOG(INFO) << "wait group braodcast coro resumed from wait...";
  };

  loop.WaitLoopEnd();
}
