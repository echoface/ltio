#ifndef BASE_IO_MULTIPLEXER_H
#define BASE_IO_MULTIPLEXER_H

#include <map>
#include <vector>

#include "fd_event.h"
#include "base/queue/double_linked_list.h"

namespace base {

typedef std::vector<FdEvent*> FdEventList;

class IOMux {
public:
  IOMux();
  virtual ~IOMux();

  virtual void AddFdEvent(FdEvent* fd_ev) = 0;
  virtual void DelFdEvent(FdEvent* fd_ev) = 0;
  virtual void UpdateFdEvent(FdEvent* fd_ev) = 0;
  virtual int WaitingIO(FdEventList& active_list, int32_t timeout_ms) = 0;

protected:
  //DoubleLinkedList<FdEvent> listen_events_;
  LinkedList<FdEvent> listen_events_;

private:
  DISALLOW_COPY_AND_ASSIGN(IOMux);
};

}
#endif
