## ltio (LigthingIO)

LigthingIO is a 'light' network IO framework with some `base` impliment for better coding experience;all of those code inspired by project Chromium/libevent/Qt/NodeJs

## code implemnet
those implemnet include follow code/component

###base
- base code
- message loop
- repeat timer
- lazyinstance
- coroutine scheduler(a limited G/M/P schedule model with work stealing)

###network io
- reactor model
- resp/line/raw/http[s]1.x protocol
- openssl tls socket implement
- raw/http[s]/line general server
- maglevHash/consistentHash/roundrobin router
- raw/http[s]/line client with full async/waitable coro
- async redis protocol[only client side support]

###component
- geo utils
- bloom filter
- countmin-sketch
- source loader (TP)
- lru/mru component
- boolean indexer(computing advertising)
- inverted indexer(computing advertising)
- add full async mysql client support; [move to ltapp]

TODO:
- websocket
- http2 implement
- RPC implement(may based on this repo to keep code clear)
+ add sendfile support for zero copy between kernel and user space

###About MessageLoop:
  like mostly messageloop implement, all PostTask/PostDelayTask/PostTaskWithReply implemented, it's inspired by chromium messageloop code;


## Build And Deploy
```bash
sudo apt-get update -yqq
sudo apt-get install -yqq \
       software-properties-common unzip \
       cmake build-essential libgoogle-perftools-dev \
       git-core libgoogle-perftools-dev libssl-dev zlib1g-dev

git clone https://github.com/echoface/ltio.git
cd ltio
git submodule update --init
mkdir build; cd build;
cmake -DWITH_OPENSSL=[ON|OFF]    \
      -DLTIO_BUILD_UNITTESTS=OFF \
      -DLTIO_USE_ACO_CORO_IMPL=OFF ../

./bin/simple_ltserver
```

## LazyInstance:
```c++
  //class Foo, only when use to create Foo Object
  static base::LazyInstance<Foo> gFoo = LAZY_INSTANCE_INIT;
```

## repeated timer:

see ut `unittests/base/repeat_timer_unittest.cc` for more detail
```
base::RepeatingTimer timer(&loop);
timer.Start(interval_ms, [&]() {
   // do something;
});
```

## TaskLoop

```c++
/*
  follow code will give you infomathion about the exception task where come from
  W1127 08:36:09.786927  9476 closure_task.h:46] Task Error Exception, From:run_loop_test.cc:24
*/

void LocationTaskTest() {
  base::MessageLoop loop;
  loop.Start();
  loop.PostTask(FROM_HERE, [](){
    throw "bad task!";
  });
  loop.WaitLoopEnd();
}

// assume a loop started
base::MessageLoop loop;

/* schedule task*/
loop.PostTask(NewClosure([&]() {
        LOG(INFO) << "delay task";
      }));
loop.PostTask(NewClosure(std::bind(...)));

loop.PostTask(FROM_HERE, [&]() {
        LOG(INFO) << "delay task";
      });
loop.PostTask(FROM_HERE, StubClass::MemberFunc, stub, ...args);

/* delay task*/
loop.PostDelayTask(NewClosure([&]() {
                    LOG(INFO) << "delay task";
                   }),500);

/* with callback notify*/
loop.PostTaskWithReply(FROM_HERE,
                       []() {/*task 1*/},
                       []() {/*task callback*/});

base::MessageLoop callback_loop;
loop.PostTaskWithReply(FROM_HERE,
                       []() {/*task 1*/},
                       []() {/*task callback*/},
                       &callback_loop,
                      );
/*callback notify task equal implement*/
loop.PostTask(FROM_HERE, [&]() {
  /* run task*/
  if (callback_loop) {
      callback_loop.PostTask(FROM_HERE, [](){
        /*run callback task*/
      })
    return;
  }
  /*run callback task*/
});

...
```

## Coroutine:

a coroutine implement base two different backend, libcoro|libaco, select one with compile
option `-DLTIO_USE_ACO_CORO_IMPL=OFF|ON`

NOTE: NO SYSTEM Hook
reason: many struggle behind this choose; not enough/perfect fundamentals implement can
make it stable work on a real world complex project with widely varying dependency 3rdparty lib

ltcoro top on base::MessageLoop, you should remember this, anther point is a started corotuine
will guarded always runing on it's binded MessageLoop(physical thread), all coro dependency things
need run on a corotine context, eg: `CO_RESUMER, CO_YIELD, CO_SLEEP, CO_SYNC`

some brief usage code looks like below:
```c++
// just imaging CO_GO equal golang's go keywords
void coro_c_function();
void coro_fun(std::string tag);
{
    CO_GO coro_c_function; //schedule c function use coroutine

    CO_GO std::bind(&coro_fun, "tag_from_go_synatax"); // c++11 bind

    CO_GO [&]() {  // lambda support
        LOG(INFO) << " run lambda in coroutine" ;
    };

    CO_GO &loop << []() { //run corotine with specify loop runing on
      LOG(INFO) << "go coroutine in loop ok!!!";
    };

    // 使用net.client定时去获取网络资源
    bool stop = false;
    CO_GO [&]() {

      CO_SYNC []() {
      };
      // equal CO_GO std::bind([](){}, CO_RESUMER());

      co_sleep(1000); // every 1s

      //self control resume, must need call resumer manually,
      //otherwise will cause corotuine leak, this useful with other async library
      auto resumer = CO_RESUMER();
      loop.PostTask(FROM_HERE, [&]() {
        resumer();
      })
    };
}

// broadcast make sure wg in a coro context
{
  CO_GO [&]() { //main
    auto wg = base::WaitGroug::New();

    wg.Add(1);
    loop.PostTask(FROM_HERE,
                  [wg]() {
                    //things
                    wg->Done();
                  });

    for (int i = 0; i < 10; i++) {
      wg.Add(1);

      CO_GO [wg]() {
        base::ScopedGuard guard([wg]() {wg->Done();});

        LOG(INFO) << "normal task start..." << l->LoopName();

        CO_SLEEP(100);
        // mock network stuff
        // request = BuildHttpRequest();
        // auto response = client.Get(request, {})
        // handle_response();
        LOG(INFO) << "normal task end..." << l->LoopName();
      };
    }

    LOG_IF(INFO, WaitGroup::kTimeout == wg->Wait(10000))
      << " timeout, not all task finish in time";
  }
```

这里没有Hook系统调用的原因主要有两个:
1. Hook系统调用的集成性并没有他们所宣传的那么好(可能我理解不够).
2. 个人仍然坚持需要知道自己在干什么,有什么风险, 开发者有选择的使用Coroutone

基于上面这个原因, 所以在ltio中, Coroutine是基于MessageLoop的TopLevel的工具. 其底层模拟实现了Golang类似的G,M,P 角色调度.并支持Worksteal, 其中有跨线程调度在C++资源管理方面带来的问题, 更重要的一点是希望通过约束其行为, 让使用着非常明确其运行的环境和作用. 从个人角度上讲, 仍旧希望他是一基于MessageLoop的Task调度为主的实现方式, 但是可以让用户根据需要使用Coroutine作为工具辅助, 使得完成一些事情来让逻辑编写更加舒适.所以有两个需要开发者了解的机制(了解就足够了)
- 1. Coroutine Task 开始运行后,以后只会在指定的物理线程切换状态 Yield-Run-End, 所以WorkSteal的语义被约束在一个全新的(schedule之后未开始运行的)可以被stealing调度到其他woker上运行, 而不是任何状态都可以被stealing调度, 任务Yield后恢复到Run状态后,仍旧在先前绑定的物理线程上;我想某些时候,你会感谢这样的实现的.😊
- 2. 调度方式两种, 作出合理的选择, 有时候这很有用:
  - `CO_GO task;` 允许这个task被workstealing的方式调度
  - `CO_GO &specified_loop << task;` 指定物理线程运行调度任务
  从我作为一个在一线业务开发多年的菜鸡选手而言, 合理的设计业务比什么都重要; 合理的选择和业务设计, 会让很多所谓的锁和资源共享变得多余; 在听到golang的口号:"不要通过共享内存来通信，而应该通过通信来共享内存"之前,本人基于chromium conenten api做开发和在计算广告设计的这几年的经验教训中对此早已有深深的体会.

## NET IO:

see `examples/net_io/simple_ltserver.cc examples/net_io/lt_http_client.cc for more detail`
---
just run a server with: `./bin/simple_ltserver`
a more benchable server: `./bin/http_benchmark_server`

a tfb benchark report will found at tfb project on next bench round
see: [tfb](https://www.techempower.com/benchmarks/)

### TLS support
- compile with `-DWITH_OPENSSL=ON`
- run simple server with selfsigned cert&key
> `./bin/simple_ltserver [--v=26] --ssl=true --cert=./cert.pem --key=./key.pem`
- use `openssl s_client` or `curl`
```
curl: with insercure(ingnore certifaction verify)
curl -v -k "https://localhost:5006/ping"

curl: with certifaction verify
curl -v --cacert ./cert.pem "https://localhost:5006/ping"

lt_http_client: without certifaction verify, insercure
./bin/lt_http_client [--v=26] --insercure=true --remote="https://localhost:5006"

lt_http_client: with certifaction verify
./bin/lt_http_client [--v=26] --insercure=true --remote="https://localhost:5006"
./bin/lt_http_client [--v=26] --cert=./cert.pem --remote="https://localhost:5006/ping"
```


NOTE
---

**email** me if any question and problem;

Learning and integrate what i leaned/think into ltio

From HuanGong 2018-01-08


# Copyright and License

Copyright (C) 2018, by HuanGong [<gonghuan.dev@gmail.com>](mailto:gonghuan.dev@gmail.com).

Under the Apache License, Version 2.0.

See the [LICENSE](LICENSE) file for details.
