#ifndef APP_CONTENT_H_H
#define APP_CONTENT_H_H

#include <base/message_loop/message_loop.h>

namespace content {

class App {
public:
  App();
  virtual ~App();

  void Run(int argc, const char** argv);
  base::MessageLoop* MainLoop();
protected:
  virtual void ContentMain() = 0;

  virtual void OnStart() {}
  virtual void OnFinish() {}
private:
  base::MessageLoop content_loop_;
};

} //namespace content
#endif
