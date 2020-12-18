
LigthingIO is a 'light' network IO framework with some `base` impliment for better coding experience;

it has implemnet follow code/component

all of those code ispired by project Chromium/libevent/Qt/NodeJs

- base code
- message loop
- repeat timer
- lazyinstance
- net io based on loop
- coroutine scheduler(a limited G/M/P schedule model with work stealing)

- raw protocol
- line protocol
- http 1.x protocol
- async redis client side support
- maglevHash/consistentHash/roundrobin router
- RPC implement(may based on this repo to keep code clear)

+ add sendfile support for zero copy between kernel and user space
+ client support full async call when task not running on coroutine task

- geo utils
- bloom filter
- countmin-sketch
- source loader (TP)

- lru/mru component
- boolean indexer(computing advertising)
- inverted indexer(computing advertising)

integration:
- add async mysql client support; [move to ltapp]

### TODO:
- net util improve(url, getaddrinfo, ioreader, iowriter, ET mode)

About MessageLoop:
  like mostly messageloop implement, all PostTask/PostDelayTask/PostTaskWithReply implemented, it's ispired by chromium messageloop code;

```c++
/*
  follow code will give you infomathion about the crash task where come from
  W1127 08:36:09.786927  9476 closure_task.h:46] Task Error Exception, From:LocationTaskTest@/path_to_project/base/message_loop/test/run_loop_test.cc:24
*/
void LocationTaskTest() {
  base::MessageLoop loop;

  loop.Start();

  loop.PostTask(NewClosure([](){
    printf("FailureDump throw failed exception");
    throw -1;
  }));

  loop.PostDelayTask(NewClosure([&]() {
    loop.QuitLoop();
  }), 2000);

  loop.WaitLoopEnd();
}
```


About Coroutine:

这是一个工具类的非系统侵入的Fullstack的Coroutine实现. 其背后有基于liaco和libcoro的两份实现.
在Ali ECS(with CentOs/Ubuntu)对比下来性能不分上下.

some brief usage code looks like below:
```c++
//the only requirement, be sure in a messageloop; it's LightingIO's fundamentals
void coro_c_function();
void coro_fun(std::string tag);
// this should be your application main
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
    CO_GO &loop << [&]() {
       do {
            // do http/redis/raw/line request async based on coroutine
            // see net/client for detail

            // do resource fetching

            co_sleep(1000); // every 1s
       } while(!stop);
    };
}

{
  MessageLoop loop;
  loop.Start();

  auto will_yield_fn = [&]() {

    // do another task async with a resume callback
    loop.PostTaskWithReply(
      NewClosure([]() {
        // some thing especial need big stack
        // Task 1
      }),
      NewClosure(co_resumer()));

    CO_YIELD; // WaitTaskEnd without block this thread;

    //scheudle another coroutine task with this coroutine's resumer
    CO_GO std::bind(AnotherCoroutineWithResume, co_resumer());

    CO_YIELD; // paused here, util co_resumer() be called in any where;
  }

  CO_GO &loop << will_yield_fn;

  loop.WaitLoopEnd();
}

{ // broadcast make sure wg in a coro context
  CO_GO [&]() { //main

    base::WaitGroup wg;

    wg.Add(1);
    loop.PostTask(FROM_HERE,
                  [&wg]() {
                    //things
                    wg.Done();
                  });

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

    LOG_IF(INFO, WaitGroup::kTimeout == wg.Wait())
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

LazyInstance:
```c++
  //class Foo, only when use to create Foo Object
  static base::LazyInstance<Foo> gFoo = LAZY_INSTANCE_INIT;
```

net performance:
---
作为对比，你可以随机的使用当前cpu支持的线程数和Nginx进行对比，不需要额外的设置. 无论你使用coroutine调度器或者非coroutine调度器。具体数据自己验证.我跑出来的数据和你跑出来的数据都不一定具有代表性，最好的测试就是你自己的测试;

repeated timer:
```
// Sample RepeatingTimer usage:
//
//   class MyClass {
//    public:
//     void StartDoingStuff() {
//       timer_.Start(ms, ClosureTask);
//     }
//     void StopStuff() {
//       timer_.Stop();
//     }
//    private:
//     void DoStuff() {
//       // This method is called every delay_ms to do stuff.
//       ...
//     }
//     base::RepeatingTimer timer_;
//   };
//

  base::RepeatingTimer timer(&loop);
  timer.Start(interval_ms, [&]() {
    // do something;
  });

```

NOTE
---

**email** me if any question and problem;

Learning and integrate what i leaned/think into ltio

From HuanGong 2018-01-08
