#include "event.h"
#include <sstream>

namespace base {

std::string events2string(const LtEvent& events) {
  std::ostringstream oss;
  oss << "[";
  if (events & LtEv::LT_EVENT_READ)
    oss << "READ ";
  if (events & LtEv::LT_EVENT_WRITE)
    oss << "WRITE ";
  if (events & LtEv::LT_EVENT_ERROR)
    oss << "ERROR ";
  if (events & LtEv::LT_EVENT_CLOSE)
    oss << "CLOSE ";
  oss << "]";
  return oss.str().c_str();
}

};
