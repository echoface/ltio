#ifndef FD_EVENT_H
#define FD_EVENT_H

#include <sys/epoll.h>
#include <sys/poll.h>

#include <functional>
#include <memory>

#include "base/base_micro.h"
#include "base/queue/linked_list.h"
#include "event.h"

namespace base {
class FdEvent;
typedef std::shared_ptr<FdEvent> RefFdEvent;
typedef std::unique_ptr<FdEvent> FdEventPtr;

/* A Event Holder and Owner represend a filedescriptor,
 * Create With a fd and take it owner  close it when it's gone*/
class FdEvent final {
public:
  /* interface notify poller notify_watcher polling events*/
  class Watcher {
  public:
    virtual ~Watcher() {}
    virtual void UpdateFdEvent(FdEvent* fd_event) = 0;
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

  static RefFdEvent Create(Handler* handler, int fd, LtEvent event);

  FdEvent(Handler* handler, int fd, LtEvent events);
  ~FdEvent();

  void SetFdWatcher(Watcher* watcher);
  Watcher* EventWatcher() {return watcher_;}

  void HandleEvent(LtEvent event);

  //the event we take care about
  LtEvent MonitorEvents() const;

  void EnableReading();
  void EnableWriting();
  void DisableReading();
  void DisableWriting();
  void SetEvent(const LtEv& lt_ev) {events_ = lt_ev; notify_watcher();}
  void DisableAll() { events_ = LtEv::LT_EVENT_NONE; notify_watcher();}
  inline bool IsReadEnable() const {return events_ & LtEv::LT_EVENT_READ;}
  inline bool IsWriteEnable() const {return events_ & LtEv::LT_EVENT_WRITE;}
  inline bool EdgeTriggerMode() const {return enable_et_;}
  inline void SetEdgeTrigger(bool edge) {enable_et_ = edge;}

  inline int fd() const {return fd_;};
  inline int GetFd() const {return fd_;};
  inline void ReleaseOwnership() {owner_fd_ = false;}

  std::string EventInfo() const;
  LtEvent ActivedEvent() const {return revents_;};
  std::string RcvEventAsString() const;
  std::string MonitorEventAsString() const;
private:
  void notify_watcher();

  const int fd_;
  LtEvent events_;
  LtEvent revents_;
  bool owner_fd_ = true;
  // for epoll mode
  bool enable_et_ = false;
  Handler* handler_ = NULL;
  Watcher* watcher_ = NULL;

  DISALLOW_COPY_AND_ASSIGN(FdEvent);
};

} // namespace
#endif // EVENT_H
