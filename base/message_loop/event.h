#ifndef BASE_LT_EVENT_H
#define BASE_LT_EVENT_H

#include <memory>

#include <functional>
#include <sys/epoll.h>
#include <sys/poll.h>
#include "base/base_micro.h"
#include "base/queue/double_linked_list.h"

namespace base {

enum LtEv {
  LT_EVENT_NONE  = 0x0000,
  LT_EVENT_READ  = 0x0001,
  LT_EVENT_WRITE = 0x0002,
  LT_EVENT_CLOSE = 0x0004,
  LT_EVENT_ERROR = 0x0008,
};

typedef uint32_t LtEvent;

std::string events2string(const LtEvent& event);

}
#endif
