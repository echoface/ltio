#ifndef FD_EVENT_H
#define FD_EVENT_H

#include <memory>
#include <functional>

#include <sys/epoll.h>
#include <sys/poll.h>

#include "event.h"
#include "base/base_micro.h"
#include "base/queue/linked_list.h"
#include "base/queue/double_linked_list.h"

namespace base {
/* A Event Holder and Owner represend a filedescriptor,
 * Create With a fd and take it owner  close it when it's gone*/
//class FdEvent : public EnableDoubleLinked<FdEvent> {
class FdEvent : public LinkedNode<FdEvent> {
public:
  /* interface notify poller notify_watcher polling events*/
  class FdEventWatcher {
  public:
    virtual ~FdEventWatcher() {}
    virtual void OnEventChanged(FdEvent* fd_event) = 0;
  };
  class Handler { //fd owner should implement those
  public:
    virtual ~Handler() {}
    // operator api, return false to block event popup
    // return true continue all event handle triggle
    virtual bool HandleRead(FdEvent* fd_event) = 0;
    virtual bool HandleWrite(FdEvent* fd_event) = 0;
    virtual bool HandleError(FdEvent* fd_event) = 0;
    virtual bool HandleClose(FdEvent* fd_event) = 0;
  };

  static std::shared_ptr<FdEvent> Create(Handler* handler, int fd, LtEvent event);

  FdEvent(Handler* handler, int fd, LtEvent events);
  ~FdEvent();

  void SetFdWatcher(FdEventWatcher *d);
  FdEventWatcher* EventWatcher() {return watcher_;}

  void HandleEvent();
  void ResetCallback();

  //the event we take care about
  LtEvent MonitorEvents() const;
  void SetRcvEvents(const LtEvent& ev);

  void SetEvent(const LtEv& lt_ev) {events_ = lt_ev; notify_watcher();}

  void EnableReading();
  inline bool IsReadEnable() const {return events_ & LtEv::LT_EVENT_READ;}

  void EnableWriting();
  inline bool IsWriteEnable() const {return events_ & LtEv::LT_EVENT_WRITE;}

  void DisableAll() { events_ = LtEv::LT_EVENT_NONE; notify_watcher();}
  void DisableReading();
  void DisableWriting();

  inline int fd() const {return fd_;};
  inline void GiveupOwnerFd() {owner_fd_ = false;}

  std::string EventInfo() const;
  LtEvent ActivedEvent() const {return revents_;};
  std::string RcvEventAsString() const;
  std::string MonitorEventAsString() const;
private:
  void notify_watcher();

  const int fd_;
  LtEvent events_;
  LtEvent revents_;
  bool owner_fd_;
  Handler* handler_ = NULL;
  FdEventWatcher* watcher_ = NULL;

  DISALLOW_COPY_AND_ASSIGN(FdEvent);
};

typedef std::shared_ptr<FdEvent> RefFdEvent;

} // namespace
#endif // EVENT_H
