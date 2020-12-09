
co_thread ct;

ct.start(func, args...);

  reusmer = makeresumer()
  co << []() {
    func,args;
    resumer();
  }

ct.join();
  YieldInternal();

std::thread thread;
thread.start(func, args...);

thread.join()
