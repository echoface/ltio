/*
 * Copyright 2021 <name of copyright holder>
 * Author: Huan.Gong <gonghuan.dev@gmail.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef FD_EVENT_H
#define FD_EVENT_H

#include <sys/epoll.h>
#include <sys/poll.h>

#include <functional>
#include <memory>

#include "base/lt_micro.h"
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
  typedef struct Watcher {
    virtual ~Watcher() = default;
    virtual void UpdateFdEvent(FdEvent* fd_event) = 0;
  } Watcher;

  typedef struct Handler {  // fd owner should implement those
    virtual ~Handler() = default;
    // operator api, return false to block event popup
    // return true continue all event handle triggle
    virtual void HandleEvent(FdEvent* fdev) = 0;
  } Handler;

  static RefFdEvent Create(Handler* handler, int fd, LtEvent event);

  FdEvent(Handler* handler, int fd, LtEvent events);
  ~FdEvent();

  void SetFdWatcher(Watcher* watcher);

  Watcher* EventWatcher() { return watcher_; }

  void Invoke(LtEvent ev);

  // the event we take care about
  LtEvent MonitorEvents() const;

  void EnableReading();
  void EnableWriting();
  void DisableReading();
  void DisableWriting();
  void SetEvent(const LtEv& lt_ev) {
    events_ = lt_ev;
    notify_watcher();
  }
  void DisableAll() {SetEvent(LtEv::LT_EVENT_NONE);}
  inline bool IsReadEnable() const { return events_ & LtEv::LT_EVENT_READ; }
  inline bool IsWriteEnable() const { return events_ & LtEv::LT_EVENT_WRITE; }

  inline bool EdgeTriggerMode() const { return enable_et_; }
  inline void SetEdgeTrigger(bool edge) { enable_et_ = edge; }

  inline int GetFd() const { return fd_; };
  inline void ReleaseOwnership() { owner_fd_ = false; }

  LtEvent ActivedEvent() const { return revents_; };
  bool RecvErr() const {return revents_ & LT_EVENT_ERROR;}
  bool RecvRead() const {return revents_ & LT_EVENT_READ;}
  bool RecvWrite() const {return revents_ & LT_EVENT_WRITE;}
  bool RecvClose() const {return revents_ & LT_EVENT_CLOSE;}

  void SetActivedEvent(LtEvent ev) {revents_ = ev;};

  std::string EventInfo() const;
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

}  // namespace base
#endif  // EVENT_H
