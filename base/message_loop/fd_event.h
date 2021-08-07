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
  struct events {
  };
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

  inline int GetFd() const { return fd_; };

  inline void ReleaseOwnership() { owner_fd_ = false; }

  void SetFdWatcher(Watcher* watcher);

  Watcher* EventWatcher() { return watcher_; }

  inline bool EdgeTriggerMode() const { return enable_et_; }

  inline void SetEdgeTrigger(bool edge) { enable_et_ = edge; }


  void EnableReading();
  void EnableWriting();
  void DisableReading();
  void DisableWriting();
  void DisableAll() {SetEvent(LtEv::LT_EVENT_NONE);}
  inline bool IsReadEnable() const { return event_ & LtEv::LT_EVENT_READ; }
  inline bool IsWriteEnable() const { return event_ & LtEv::LT_EVENT_WRITE; }

  // set event user intresting
  void SetEvent(const LtEv& ev);
  // the event user take care about
  LtEvent MonitorEvents() const {return event_;};
  std::string MonitorEventStr() const{return ev2str(event_);};


  void Invoke(LtEvent ev);

  LtEvent ActivedEvent() const { return fired_; };
  bool ErrFired() const {return fired_ & LT_EVENT_ERROR;}
  bool ReadFired() const {return fired_ & LT_EVENT_READ;}
  bool WriteFired() const {return fired_ & LT_EVENT_WRITE;}
  bool CloseFired() const {return fired_ & LT_EVENT_CLOSE;}
  std::string FiredEventStr() const { return ev2str(fired_);};

  std::string EventInfo() const;
private:
  void notify_watcher();

  const int fd_ = -1;

  bool owner_fd_ = true;

  LtEvent event_ = LT_EVENT_NONE; // watching event
  LtEvent fired_ = LT_EVENT_NONE; // fired event

  // for epoll mode
  bool enable_et_ = false;

  Handler* handler_ = NULL;

  Watcher* watcher_ = NULL;

  DISALLOW_COPY_AND_ASSIGN(FdEvent);
};

}  // namespace base
#endif  // EVENT_H
