#include "coroutine.h"

namespace base {

std::string StateToString(CoroState st) {
  switch(st) {
    case CoroState::kDone:
      return "Done";
    case CoroState::kPaused:
      return "Paused";
    case CoroState::kRunning:
      return "Running";
    case CoroState::kInit:
      return "Initialized";
    default:
      break;
  }
  return "Unknown";
}

} // end base
