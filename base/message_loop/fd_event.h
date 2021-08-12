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
#include "base/queue/double_linked_list.h"
#include "event.h"

namespace base {
class FdEvent;
using RefFdEvent = std::shared_ptr<FdEvent>;
using FdEventPtr = std::unique_ptr<FdEvent>;

/* A Event Holder and Owner represend a filedescriptor,
 * Create With a fd and take it owner close it when it's gone*/
class FdEvent final {
public:
  struct events {};
  /* interface notify iomux polling events*/
  typedef struct Watcher {
    virtual ~Watcher() = default;

    virtual void UpdateFdEvent(FdEvent* fd_event) = 0;
  } Watcher;

  typedef struct Handler {  // fd owner should implement those
    virtual ~Handler() = default;

    // operator api, return false to block event popup
    // return true continue all event handle triggle
    virtual void HandleEvent(FdEvent* fdev, LtEv::Event ev) = 0;
  } Handler;

  static RefFdEvent Create(int fd, LtEv::Event ev);

  static RefFdEvent Create(Handler* h, int fd, LtEv::Event ev);

  FdEvent(int fd, LtEv::Event events);

  FdEvent(Handler* handler, int fd, LtEv::Event ev);

  ~FdEvent();

  inline int GetFd() const { return fd_; };

  inline void SetHandler(Handler* h) { handler_ = h; }

  inline Handler* GetHandler() const { return handler_;}

  inline void SetFdWatcher(Watcher* w) {watcher_ = w;}

  inline Watcher* EventWatcher() { return watcher_; }

  inline void ReleaseOwnership() { owner_fd_ = false; }

  inline bool EdgeTriggerMode() const { return enable_et_; }

  inline void SetEdgeTrigger(bool edge) { enable_et_ = edge; }

  void EnableReading();
  void EnableWriting();
  void DisableReading();
  void DisableWriting();
  void DisableAll() { SetEvent(LtEv::NONE); }
  inline bool IsReadEnable() const { return event_ & LtEv::READ; }
  inline bool IsWriteEnable() const { return event_ & LtEv::WRITE; }

  // set event user intresting
  void SetEvent(LtEv::Event ev);

  // the event user take care about
  LtEv::Event MonitorEvents() const { return event_; };

  LtEv::Event FiredEvent() const { return fired_; };

  void Invoke(LtEv::Event ev);

  std::string EventInfo() const;

private:
  void notify_watcher();

  const int fd_ = -1;

  bool owner_fd_ = true;

  LtEv::Event event_ = LtEv::NONE;
  LtEv::Event fired_ = LtEv::NONE;

  // for epoll mode
  bool enable_et_ = false;

  Handler* handler_ = NULL;

  Watcher* watcher_ = NULL;

  DISALLOW_COPY_AND_ASSIGN(FdEvent);
};

}  // namespace base
#endif  // EVENT_H
