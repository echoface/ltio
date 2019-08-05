
#include "io_multiplexer.h"

namespace base {

IOMux::IOMux() {
}
IOMux::~IOMux() {
}

uint32_t IOMux::WatchingFdCounts() const {
  return listen_events_.Size();
}

}
