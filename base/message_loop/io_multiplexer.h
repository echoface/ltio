#ifndef BASE_IO_MULTIPLEXER_H
#define BASE_IO_MULTIPLEXER_H

#include <map>
#include <vector>

#include "event.h"
#include "fd_event.h"

namespace base {

typedef struct FiredEvent {
  int fd_id;
  LtEvent event_mask;
  void reset() {fd_id = -1; event_mask = LT_EVENT_NONE;} 
}FiredEvent;

class IOMux : public FdEvent::Watcher {
public:
  IOMux();
  virtual ~IOMux();

  virtual int WaitingIO(FiredEvent* start, int32_t timeout_ms) = 0;

  virtual void AddFdEvent(FdEvent* fd_ev) = 0;
  virtual void DelFdEvent(FdEvent* fd_ev) = 0;

  // override for watch event update
  virtual void UpdateFdEvent(FdEvent* fd_ev) override = 0;

  /* may return NULL if removed by
   * previous InvokedEvent handler*/
  virtual FdEvent* FindFdEvent(int fd) = 0;
private:
  DISALLOW_COPY_AND_ASSIGN(IOMux);
};

}
#endif
