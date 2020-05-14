#ifndef BASE_IO_MULTIPLEXER_EPOLL_H
#define BASE_IO_MULTIPLEXER_EPOLL_H

#include "event.h"
#include "fd_event.h"
#include "io_multiplexer.h"
#include <array>

namespace base {

class IOMuxEpoll : public IOMux {
public:
  IOMuxEpoll(int32_t max_fds);
  ~IOMuxEpoll();

  FdEvent* FindFdEvent(int fd) override;
  void AddFdEvent(FdEvent* fd_ev) override;
  void DelFdEvent(FdEvent* fd_ev) override;
  void UpdateFdEvent(FdEvent* fd_ev) override;

  int WaitingIO(FiredEvent* active_list, int32_t timeout_ms) override;
private:
  int EpollCtl(FdEvent* ev, int opt);
  LtEvent ToLtEvent(const uint32_t epoll_ev);
  uint32_t ToEpollEvent(const LtEvent& lt_ev, bool add_extr = true);
  std::string EpollOptToString(int opt);
private:
  int epoll_fd_ = -1;
  std::vector<FdEvent*> lt_events_;
  std::vector<epoll_event> ep_events_;
};

}
#endif
