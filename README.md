
LigthingIO is a 'light' network IO application framework with some `base` impliment for better coding experience;

it has implemnet follow code/component

- base code
- message loop
- repeat timer
- coroutine scheduler
- lazyinstance

- net io based on loop

- raw protocol
- line protocol
- http 1.x protocol
- async redis client side support

+ add sendfile support for zero copy between kernel and user space
+ client support full async call when task not running on coroutine task

- thread safe lru component
- inverted indexer table
- count min sketch with its utils
- bloom filter
- source loader (TP)

integration:
- add async mysql client support; [pre]

About MessageLoop:
  like mostly messageloop class, all PostTask/PostDelayTask/PostTaskWithReply implemented, it's ispired by chromium messageloop code; it's task also support location info for more convinience debug

```c++
/*
  follow code will give you infomathion about the crash task where from
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
coroutine sheduler was a 'none-system-hack' type implement, why?, most of `blablabla wonderful coroutine library` will has some not clear behavior,(eg: thread locale storage. cross thread schedule, not-compatiable system api hack); the coroutine schedule implement depend a clear resume and run condition youself control; only work in a thread your arranged;
chinese:
coroutine调度的实现依赖于message loop， 没有对系统调用进行hack， 往往那些实现了很NB的coroutine库看上去的确很牛逼， 但是大多数却有着他们没有提到的这样或者那样的问题。比如说跨线程调度/thread locale storage的支持. 系统api的兼容等等。这些问题上这些库往往一言带过或者避而不谈。我遇到的95%甚至更多的coroutine库都有thread locale storage的问题;谁能保证我们引入的第三方代码里面没有使用呢？有些实现NB的库实现了coroutine local storage的确很叼，谁能保证我们引入的第三方代码里面没有使用呢？在C++没有垃圾回收机制的情况下。其实这些行为都非常危险; LightingIO中的现实没有syscall的hack， 不支持自动的跨线程调度，支持使用TLS（本身也依赖TLS）。他的切换与唤起需要MessageLoop和coroutine本身的配合，看上去不那么自动化，但是能非常符合我们预期的进行控制. 本质上,corotine 只是MessageLoop的一个TaskRunner; 但是我们却没有使用Crorontine作为任务队列的默认Runner。其实对于任何stackfull的coroutine来说，都是有代价的。而大多数时候，我们并不需要这些corutine，只是在某些必要的场景下， 我们希望很好的切换来使得代码更加可读，可控;特别对我这样的菜鸟级选手而言
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

    // do something here util has something need async done
    // 例如这件事情需要很大的栈空间来完成或者是需要异步完成，这里挂起当前coroutine，并在这里就指定好resume的逻辑
    loop.PostTaskWithReply(NewClosure([]() {
      // some thing especial need big stack
    }),
    NewClosure(co_resumer));

    //or another function in coroutine with current corotine's resume_closure
    co_go std::bind(AnotherCoroutineWithResume, co_resumer);

    co_yield; // paused here, util co_resumer be called in any where;

    //  do other things
  }

  //keep in loop run this code
  loop.PostTask(NewClosure([&]() {
    co_go will_yield_fn; //just schedule, not entry now
  });
  loop.WaitLoopEnd();
}
```

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
  INPROCESS;

  This library still in progress, do not use it in your project or production environments;
  **email** me if any question and problem;

Learning and integrate what i think into LightingIO

LightingIO itself just a normal network application framework; all its technology from my practice and work

at the start time of this project; i use libevent, but i find many things was limited by the exsiting code; it's hard to hack or change
the libevent code; so... I give up libvent at last, and then start all things from zero, in thoes days and night, thanks the support from
my girl friends; and other reference books


From HuanGong 2018-01-08
