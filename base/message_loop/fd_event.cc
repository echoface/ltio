#include "fd_event.h"
#include "glog/logging.h"
#include "base/base_constants.h"

namespace base {

RefFdEvent FdEvent::Create(int fd, uint32_t events) {
  return std::make_shared<FdEvent>(fd, events);
}

FdEvent::FdEvent(int fd, uint32_t event):
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

uint32_t FdEvent::MonitorEvents() const {
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

void FdEvent::SetRcvEvents(uint32_t ev) {
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
  events_ |= EPOLLIN;
  Update();
}

void FdEvent::EnableWriting() {
  if (IsWriteEnable()) {
    return;
  }
  events_ |= EPOLLOUT;
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
    if (revents_ & EPOLLERR) {
      if (read_callback_) {
        error_callback_();
      }
      break;
    }

    if (revents_ & EPOLLHUP) {
      if (read_callback_) {
        read_callback_();
      }
      break;
    }

    if (revents_ & EPOLLIN) {
      if (read_callback_) {
        read_callback_();
      }
    }

    if (revents_ & EPOLLOUT) {
      if (write_callback_) {
        write_callback_();
      }
    }

    if (revents_ & EPOLLRDHUP) { //peer close
      if (close_callback_) {
        close_callback_();
      }
    }
  } while(0);

  revents_ = 0;
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

std::string FdEvent::events2string(uint32_t events) const {
  std::ostringstream oss;
  oss << "fd: " << fd_ << ": [";
  if (events & EPOLLIN)
    oss << "IN ";
  if (events & EPOLLPRI)
    oss << "PRI ";
  if (events & EPOLLOUT)
    oss << "OUT ";
  if (events & EPOLLHUP)
    oss << "HUP ";
  if (events & EPOLLRDHUP)
    oss << "RDHUP ";
  if (events & EPOLLERR)
    oss << "ERR ";
  if (events & POLLNVAL)
    oss << "NVAL";
  oss << "]";
  return oss.str().c_str();
}

} //namespace
