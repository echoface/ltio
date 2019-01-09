#include "fd_event.h"
#include "glog/logging.h"
#include "base/base_constants.h"

namespace base {

RefFdEvent FdEvent::Create(int fd, LtEvent events) {
  return std::make_shared<FdEvent>(fd, events);
}

FdEvent::FdEvent(int fd, LtEvent event):
  fd_(fd),
  events_(event),
  revents_(0),
  owner_fd_life_(true) {
}

FdEvent::~FdEvent() {
  if (owner_fd_life_) {
    ::close(fd_);
  }
}

void FdEvent::SetFdWatcher(FdEventWatcher *d) {
  watcher_ = d;
}

LtEvent FdEvent::MonitorEvents() const {
  return events_;
}

void FdEvent::SetReadCallback(const EventCallback &cb) {
  read_callback_ = cb;
}

void FdEvent::SetWriteCallback(const EventCallback &cb) {
  write_callback_ = cb;
}

void FdEvent::SetErrorCallback(const EventCallback &cb) {
  error_callback_ = cb;
}

void FdEvent::SetRcvEvents(const LtEvent& ev) {
  revents_ = ev;
}

void FdEvent::ResetCallback() {
  static const EventCallback kNullCallback;
  error_callback_ = kNullCallback;
  write_callback_ = kNullCallback;
  read_callback_  = kNullCallback;
  close_callback_ = kNullCallback;
}

void FdEvent::EnableReading() {
  if (IsReadEnable()) {
    return;
  }
  events_ |= LtEv::LT_EVENT_READ;
  Update();
}

void FdEvent::EnableWriting() {
  if (IsWriteEnable()) {
    return;
  }
  events_ |= LtEv::LT_EVENT_WRITE;
  Update();
}

void FdEvent::Update() {
  if (watcher_) {
    watcher_->OnEventChanged(this);
  }
  VLOG(GLOG_VTRACE) << __FUNCTION__ << EventInfo() << " updated";
}

void FdEvent::SetCloseCallback(const EventCallback &cb) {
  close_callback_ = cb;
}

void FdEvent::HandleEvent() {

  VLOG(GLOG_VTRACE) << __FUNCTION__ << EventInfo() << " rcv event:" << RcvEventAsString();
  do {
    if (revents_ & LtEv::LT_EVENT_ERROR) {
      if (read_callback_) {
        error_callback_();
      }
      break;
    }

    if (revents_ & LtEv::LT_EVENT_WRITE) {
      if (write_callback_) {
        write_callback_();
      }
    }

    if (revents_ & LtEv::LT_EVENT_READ) {
      if (read_callback_) {
        read_callback_();
      }
    }

    if (revents_ & LtEv::LT_EVENT_CLOSE) {
      if (close_callback_) {
        close_callback_();
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
