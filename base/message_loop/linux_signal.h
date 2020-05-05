#ifndef SIGNAL_H
#define SIGNAL_H

#include <functional>
#include <initializer_list>
#include <sys/types.h>
#include <signal.h>
#include <sys/signalfd.h>

namespace base {

//signal(SIGPIPE, SIG_IGN);
void IgnoreSigPipeSignalOnCurrentThread();

int CreateNoneBlockingSignalFd(std::initializer_list<int> signals);

struct Signal {
    static void signal(int sig, const std::function<void()>& handler);
};


} //namespace

#endif // SIGNAL_H
