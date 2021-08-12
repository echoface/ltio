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

RefFdEvent FdEvent::Create(int fd, LtEv::Event ev) {
  return std::make_shared<FdEvent>(nullptr, fd, ev);
}

RefFdEvent FdEvent::Create(FdEvent::Handler* h, int fd, LtEv::Event ev) {
  return std::make_shared<FdEvent>(h, fd, ev);
}

FdEvent::FdEvent(int fd, LtEv::Event events) : FdEvent(nullptr, fd, events) {}

FdEvent::FdEvent(FdEvent::Handler* handler, int fd, LtEv::Event event)
  : fd_(fd),
    event_(event),
    fired_(0),
    owner_fd_(true),
    handler_(handler) {}

FdEvent::~FdEvent() {
  if (owner_fd_) {
    ::close(fd_);
  }
}

void FdEvent::EnableReading() {
  if (IsReadEnable()) {
    return;
  }
  event_ |= LtEv::READ;
  notify_watcher();
}

void FdEvent::EnableWriting() {
  if (IsWriteEnable()) {
    return;
  }
  event_ |= LtEv::WRITE;
  notify_watcher();
}

void FdEvent::DisableReading() {
  if (!IsReadEnable())
    return;

  event_ &= ~LtEv::READ;
  notify_watcher();
}

void FdEvent::DisableWriting() {
  if (!IsWriteEnable())
    return;

  event_ &= ~LtEv::WRITE;
  notify_watcher();
}

void FdEvent::SetEvent(LtEv::Event ev) {
  event_ = ev;
  notify_watcher();
}

void FdEvent::notify_watcher() {
  if (watcher_) {
    watcher_->UpdateFdEvent(this);
  }
}

void FdEvent::Invoke(LtEv::Event ev) {
  fired_ = ev;
  VLOG(GLOG_VTRACE) << EventInfo();
  handler_->HandleEvent(this, ev);
}

std::string FdEvent::EventInfo() const {
  std::ostringstream oss;
  oss << " [fd:" << fd_ << ", watch:" << LtEv::to_string(event_)
      << ", fired:" << LtEv::to_string(fired_) << "]";
  return oss.str();
}

}  // namespace base
