#ifndef FD_EVENT_H
#define FD_EVENT_H

#include <memory>
#include <functional>

#include <sys/epoll.h>
#include <sys/poll.h>

#include "event.h"
#include "base/base_micro.h"
#include "base/queue/double_linked_list.h"

namespace base {
/* A Event Holder and Owner represend a filedescriptor,
 * Create With a fd and take it owner  close it when it's gone*/
class FdEvent : public EnableDoubleLinked<FdEvent> {
public:
  /* interface notify poller notify_watcher polling events*/
  class FdEventWatcher {
  public:
    virtual ~FdEventWatcher() {}
    virtual void OnEventChanged(FdEvent* fd_event) = 0;
  };
  class EventHandler { //fd owner should implement those
  public:
    virtual ~EventHandler() {}
    virtual void HandleRead(FdEvent* fd_event) = 0;
    virtual void HandleWrite(FdEvent* fd_event) = 0;
    virtual void HandleError(FdEvent* fd_event) = 0;
    virtual void HandleClose(FdEvent* fd_event) = 0;
  };

  typedef std::function<void()> EventCallback;
  static std::shared_ptr<FdEvent> Create(int fd, LtEvent event);

  FdEvent(int fd, LtEvent events);
  ~FdEvent();

  void SetFdWatcher(FdEventWatcher *d);
  FdEventWatcher* EventWatcher() {return watcher_;}

  void HandleEvent();
  void ResetCallback();
  void SetReadCallback(const EventCallback &cb);
  void SetWriteCallback(const EventCallback &cb);
  void SetErrorCallback(const EventCallback &cb);
  void SetCloseCallback(const EventCallback &cb);

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
  inline void GiveupOwnerFd() {owner_fd_life_ = false;}

  std::string EventInfo() const;
  std::string RcvEventAsString() const;
  std::string MonitorEventAsString() const;
private:
  void notify_watcher();

  const int fd_;
  LtEvent events_;
  LtEvent revents_;
  bool owner_fd_life_;

  EventHandler* handler_ = NULL;
  FdEventWatcher* watcher_ = NULL;

  EventCallback read_callback_;
  EventCallback write_callback_;
  EventCallback error_callback_;
  EventCallback close_callback_;
  DISALLOW_COPY_AND_ASSIGN(FdEvent);
};

typedef std::shared_ptr<FdEvent> RefFdEvent;

} // namespace
#endif // EVENT_H
