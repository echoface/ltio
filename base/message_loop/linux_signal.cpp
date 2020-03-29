#include <map>
#include <functional>

#include "linux_signal.h"

namespace base {

void IgnoreSigPipeSignalOnCurrentThread() {
  sigset_t sigpipe_mask;
  sigemptyset(&sigpipe_mask);
  sigaddset(&sigpipe_mask, SIGPIPE);
  pthread_sigmask(SIG_BLOCK, &sigpipe_mask, nullptr);
}

std::map<int, std::function<void()>> handlers;

void signal_handler(int sig) {
    handlers[sig]();
}

void Signal::signal(int sig, const std::function<void ()> &handler) {
    handlers[sig] = handler;
    ::signal(sig, signal_handler);
}

}
