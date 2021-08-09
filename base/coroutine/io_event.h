#ifndef _LT_CORO_IO_EVENT_H_
#define _LT_CORO_IO_EVENT_H_

#include "base/closure/closure_task.h"
#include "base/lt_micro.h"
#include "base/message_loop/event.h"
#include "base/message_loop/event_pump.h"
#include "base/message_loop/fd_event.h"

namespace co {

/*
 * IMPORTANT: all thing in base/coroutine need live in coro context
 *
 * co::IOEvent use for wait io ready
 *
 * IOEvent ioev(int fd, LtEv event);
 * IOEvent ioev(base::FdEvent fdev);
 *
 * auto ret = ioev.wait(int ms);
 *
 * */
class IOEvent : public base::FdEvent::Handler {
public:
  enum Result {
    None    = 0,
    Ok      = 1,
    Error   = 2,
    Timeout = 4,
  };

  IOEvent(base::FdEvent* fdev);

  ~IOEvent();

  std::string ResultStr() const;

  /*
   * wait here till a fd event happened
   *
   * fd must be a non-blocking file-descript
   * */
  __CO_WAIT__ Result Wait(int ms = -1);

protected:
  void initialize();

  void HandleEvent(base::FdEvent* fdev) override;
private:
  void set_result(Result res) {
    if (result_ == None) {
      result_ = res;
      return;
    }
  }

  Result result_ = None;
  base::EventPump* pump_;
  base::FdEvent* fd_event_;
  base::LtClosure resumer_;

  DISALLOW_ALLOCT_HEAP_OBJECT;
  DISALLOW_COPY_AND_ASSIGN(IOEvent);
};

}  // namespace co
#endif
