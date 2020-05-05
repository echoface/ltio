#include "fd_event.h"
#include "glog/logging.h"
#include "base/base_constants.h"

namespace base {

RefFdEvent FdEvent::Create(FdEvent::Handler* handler, int fd, LtEvent events) {
  return std::make_shared<FdEvent>(handler, fd, events);
}

FdEvent::FdEvent(FdEvent::Handler* handler, int fd, LtEvent event):
  fd_(fd),
  events_(event),
  revents_(0),
  owner_fd_(true),
  handler_(handler) {
}

FdEvent::~FdEvent() {
  if (owner_fd_) {
    ::close(fd_);
  }
}

void FdEvent::SetFdWatcher(FdEventWatcher *d) {
  watcher_ = d;
}

LtEvent FdEvent::MonitorEvents() const {
  return events_;
}

void FdEvent::SetRcvEvents(const LtEvent& ev) {
  revents_ = ev;
}

void FdEvent::ResetCallback() {
  handler_ = nullptr;
}

void FdEvent::EnableReading() {
  if (IsReadEnable()) {
    return;
  }
  events_ |= LtEv::LT_EVENT_READ;
  notify_watcher();
}

void FdEvent::EnableWriting() {
  if (IsWriteEnable()) {
    return;
  }
  events_ |= LtEv::LT_EVENT_WRITE;
  notify_watcher();
}

void FdEvent::DisableReading() {
  if (!IsReadEnable()) return;
  events_ &= ~LtEv::LT_EVENT_READ;
  notify_watcher();
}

void FdEvent::DisableWriting() {
  if (!IsWriteEnable()) return;
  events_ &= ~LtEv::LT_EVENT_WRITE;
  notify_watcher();
}

void FdEvent::notify_watcher() {
  if (watcher_) {
    watcher_->OnEventChanged(this);
  }
  VLOG_IF(GLOG_VTRACE, watcher_ == NULL) << " no watcher monitor this event";
}

void FdEvent::HandleEvent() {
  VLOG(GLOG_VTRACE) << __FUNCTION__ << EventInfo() << " rcv event:" << RcvEventAsString();
  do {
    if (revents_ & LtEv::LT_EVENT_ERROR) {
      if (handler_) {
        handler_->HandleError(this);
      }
      break;
    }

    if (revents_ & LtEv::LT_EVENT_WRITE) {
      if (handler_) {
        handler_->HandleWrite(this);
      }
    }

    if (revents_ & LtEv::LT_EVENT_READ) {
      if (handler_) {
        handler_->HandleRead(this);
      }
    }

    if (revents_ & LtEv::LT_EVENT_CLOSE) {
      if (handler_) {
        handler_->HandleClose(this);
      }
    }
  } while(0);
  revents_ = LtEv::LT_EVENT_NONE;
}

std::string FdEvent::EventInfo() const {
  std::ostringstream oss;
  oss << " [fd:" << fd()
      << ", monitor:" << MonitorEventAsString() << "]";
  return oss.str();
}

std::string FdEvent::RcvEventAsString() const {
  return events2string(revents_);
}

std::string FdEvent::MonitorEventAsString() const {
  return events2string(events_);
}

} //namespace
