
LigthingIO is a 'light' network IO framework with some `base` impliment for better coding experience;

it has implemnet follow code/component

all of those code ispired by project Chromium/libevent/Qt/NodeJs

- base code
- message loop
- repeat timer
- lazyinstance
- coroutine scheduler

- net io based on loop

- raw protocol
- line protocol
- http 1.x protocol
- async redis client side support

+ add sendfile support for zero copy between kernel and user space
+ client support full async call when task not running on coroutine task

- thread safe lru component
- inverted indexer table
- bloom filter
- count min sketch with its utils
- source loader (TP)

integration:
- add async mysql client support; [pre]

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
    co_go coro_c_function; //schedule c function use coroutine

    co_go std::bind(&coro_fun, "tag_from_go_synatax"); // c++11 bind

    co_go [&]() {  // lambda support
        LOG(INFO) << " run lambda in coroutine" ;
    };

    co_go &loop << []() { //run corotine with specify loop runing on
      LOG(INFO) << "go coroutine in loop ok!!!";
    };

    // 使用net.client定时去获取网络资源
    bool stop = false;
    co_go &loop << [&]() {
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
    co_pause; // WaitTaskEnd without block this thread;


    //scheudle another coroutine task with this coroutine's resumer
    co_go std::bind(AnotherCoroutineWithResume, co_resumer());
    co_pause; // paused here, util co_resumer() be called in any where;
  }

  co_go &loop << will_yield_fn;

  loop.WaitLoopEnd();
}
```
这里没有Hook系统调用的原因主要有两个:
1. Hook系统调用的集成性并没有他们所宣传的那么好(可能我理解不够).
2. 个人仍然坚持需要知道自己在干什么,有什么风险的开发者有选择的使用Coroutone

基于上面这个原因, 所以在ltio中, Coroutine是基于MessageLoop的TopLevel的工具. 其底层模拟实现了Golang类似的G,M,P 角色调度.
不同点在于并没有支持Worksteal, 其中的一些原因有跨线程调度在C++资源管理方面带来的问题.跟重要的一点是希望通过约束其行为
让使用着非常明确其运行的环境和作用. 如果有需要可以基于以固定Loop数量(eg: CPU*2)为Pool的基础上Wrap一个WorkSteal的封装.
从个人角度上讲, 仍旧希望他是一基于MessageLoop的Task调度为主的实现方式, 但是可以让用户根据需要使用Coroutine作为工具辅助
完成一些事情来让这个过程更加舒适.


LazyInstance:
```c++
  //class Foo, only when use to create Foo Object
  static base::LazyInstance<Foo> gFoo = LAZY_INSTANCE_INIT;
```

net performance:
---
作为对比，你可以随机的使用当前cpu支持的线程数和Nginx进行对比，不需要额外的设置. 无论你使用coroutine调度器
或者非coroutine调度器。具体数据自己验证.我跑出来的数据和你跑出来的数据都不一定具有代表性，最好的测试就是你
自己的测试;

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
