#include "glog/logging.h"
#include <base/utils/sys_error.h>

#include "io_multiplexer.h"
#include "io_mux_epoll.h"

namespace base {

static const int kMax_EPOLL_FD_NUM = 65535;

IOMuxEpoll::IOMuxEpoll(int32_t max_fds)
  : IOMux(),
    lt_events_(max_fds, NULL),
    ep_events_(kMax_EPOLL_FD_NUM) {

  epoll_fd_ = ::epoll_create(kMax_EPOLL_FD_NUM);
}

IOMuxEpoll::~IOMuxEpoll() {
  ::close(epoll_fd_);
}

int IOMuxEpoll::WaitingIO(FiredEvent* active_list, int32_t timeout_ms) {

  int turn_active_count = ::epoll_wait(epoll_fd_,
                                       &ep_events_[0],
                                       kMax_EPOLL_FD_NUM,
                                       timeout_ms);

  if (turn_active_count < 0) {//error
    int32_t err = errno;
    LOG(ERROR) << "epoll fd:" << epoll_fd_ << " epoll_wait error:" << StrError(err);
    return 0;
  }

  for (int idx = 0; idx < turn_active_count; idx++) {
    struct epoll_event& ev = ep_events_[idx];
    //DCHECK(fd_ev);
    DCHECK(lt_events_[ev.data.fd] != NULL);

    active_list[idx].fd_id = ev.data.fd;
    active_list[idx].event_mask = ToLtEvent(ev.events);
  }
  return turn_active_count;
}

LtEvent IOMuxEpoll::ToLtEvent(const uint32_t epoll_ev) {
  LtEvent event = LtEv::LT_EVENT_NONE;

  if (epoll_ev & EPOLLERR) {
    event |=  LtEv::LT_EVENT_ERROR;
  }
  //case readable:
  //case hang out: but can read till EOF
  if (epoll_ev & EPOLLHUP || epoll_ev & EPOLLIN) {
    event |=  LtEv::LT_EVENT_READ;
  }
  //writable
  if (epoll_ev & EPOLLOUT) {
    event |=  LtEv::LT_EVENT_WRITE;
  }
  //peer close
  if (epoll_ev & EPOLLRDHUP) {
    event |= LtEv::LT_EVENT_CLOSE;
  }
  return event;
}

uint32_t IOMuxEpoll::ToEpollEvent(const LtEvent& lt_ev, bool add_extr) {
  uint32_t epoll_ev = 0;
  if (lt_ev & LtEv::LT_EVENT_READ) {
    epoll_ev |= EPOLLIN;
  }
  if (lt_ev & LtEv::LT_EVENT_WRITE) {
    epoll_ev |= EPOLLOUT;
  }
  if ((lt_ev & LtEv::LT_EVENT_CLOSE) || add_extr) {
    epoll_ev |= EPOLLRDHUP;
  }
  return epoll_ev;
}

FdEvent* IOMuxEpoll::FindFdEvent(int fd) {
  if (fd >= 0 && fd < lt_events_.size()) {
    return lt_events_[fd];
  }
  LOG(ERROR) << __FUNCTION__ << " out of range file descriptor";
  return NULL;
}

void IOMuxEpoll::AddFdEvent(FdEvent* fd_ev) {
  CHECK((fd_ev->fd() >= 0) && (fd_ev->fd() < lt_events_.size()));

  EpollCtl(fd_ev, EPOLL_CTL_ADD);

  int32_t fd = fd_ev->fd();
  if (lt_events_[fd] != NULL && lt_events_[fd] != fd_ev) {
    LOG(ERROR) << __FUNCTION__ << " override a exsist fdevent"
      << ", origin:" << lt_events_[fd] << " new:" << fd_ev;
  }
  lt_events_[fd] = fd_ev;
}

void IOMuxEpoll::DelFdEvent(FdEvent* fd_ev) {

  EpollCtl(fd_ev, EPOLL_CTL_DEL);

  int32_t fd = fd_ev->fd();
  if (lt_events_[fd] == fd_ev) {
    lt_events_[fd] = NULL;
  } else {
    LOG(ERROR) << "Attept Remove A None-Exsist FdEvent";
  }
}

void IOMuxEpoll::UpdateFdEvent(FdEvent* fd_ev) {
  if (0 != EpollCtl(fd_ev, EPOLL_CTL_MOD)) {
    LOG(ERROR) << "update fd event failed:" << fd_ev->EventInfo();
    return;
  }
  int32_t fd = fd_ev->fd();
  if (lt_events_[fd] == NULL) {
    lt_events_[fd] = fd_ev;
  }
}

int IOMuxEpoll::EpollCtl(FdEvent* fdev, int opt) {
  struct epoll_event ev;

  int fd = fdev->fd();

  ev.data.fd = fd;
  ev.events = ToEpollEvent(fdev->MonitorEvents());

  int ret = ::epoll_ctl(epoll_fd_, opt, fd, &ev);

  LOG_IF(ERROR, ret != 0) << "apply epoll_ctl opt " << EpollOptToString(opt)
    << " on fd " << fd << " failed, errno:" << StrError(errno)
    << " events:" << fdev->MonitorEventAsString();

  return ret;
}

std::string IOMuxEpoll::EpollOptToString(int opt) {
  switch(opt) {
    case EPOLL_CTL_ADD:
      return "EPOLL_CTL_ADD";
    case EPOLL_CTL_DEL:
      return "EPOLL_CTL_DEL";
    case EPOLL_CTL_MOD:
      return "EPOLL_CTL_MOD";
    default:
      break;
  }
  return "BAD CTL OPTION";
}

}
