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

#include "fd_event.h"

#include "glog/logging.h"

#include "base/base_constants.h"

namespace base {

RefFdEvent FdEvent::Create(FdEvent::Handler* handler, int fd, LtEvent events) {
  return std::make_shared<FdEvent>(handler, fd, events);
}

FdEvent::FdEvent(FdEvent::Handler* handler, int fd, LtEvent event)
  : fd_(fd), event_(event), fired_(0), owner_fd_(true), handler_(handler) {}

FdEvent::~FdEvent() {
  if (owner_fd_) {
    ::close(fd_);
  }
}

void FdEvent::SetFdWatcher(Watcher* d) {
  watcher_ = d;
}

void FdEvent::EnableReading() {
  if (IsReadEnable()) {
    return;
  }
  event_ |= LtEv::LT_EVENT_READ;
  notify_watcher();
}

void FdEvent::EnableWriting() {
  if (IsWriteEnable()) {
    return;
  }
  event_ |= LtEv::LT_EVENT_WRITE;
  notify_watcher();
}

void FdEvent::DisableReading() {
  if (!IsReadEnable())
    return;

  event_ &= ~LtEv::LT_EVENT_READ;
  notify_watcher();
}

void FdEvent::DisableWriting() {
  if (!IsWriteEnable())
    return;

  event_ &= ~LtEv::LT_EVENT_WRITE;
  notify_watcher();
}

void FdEvent::SetEvent(const LtEv& ev) {
  event_ = ev;
  notify_watcher();
}

void FdEvent::notify_watcher() {
  if (watcher_) {
    watcher_->UpdateFdEvent(this);
  }
}

void FdEvent::Invoke(LtEvent ev) {
  fired_ = ev;
  VLOG(GLOG_VTRACE) << EventInfo();
  handler_->HandleEvent(this);
  fired_ = LT_EVENT_NONE;
}

std::string FdEvent::EventInfo() const {
  std::ostringstream oss;
  oss << " [fd:" << fd_
    << ", watch:" << ev2str(event_)
    << ", fired:" << ev2str(fired_) << "]";
  return oss.str();
}

}  // namespace base
