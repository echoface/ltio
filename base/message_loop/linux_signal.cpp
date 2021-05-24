/*
 * Copyright 2021 <name of copyright holder>
 * Author: Huan.Gong <gonghuan.dev@gmail.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <map>
#include <functional>

#include "linux_signal.h"

namespace base {

std::map<int, std::function<void()>> handlers;
void signal_handler(int sig) {
    handlers[sig]();
}

void Signal::signal(int sig, const std::function<void ()> &handler) {
    handlers[sig] = handler;
    ::signal(sig, signal_handler);
}

void IgnoreSigPipeSignalOnCurrentThread() {
  sigset_t sigpipe_mask;
  sigemptyset(&sigpipe_mask);
  sigaddset(&sigpipe_mask, SIGPIPE);
  pthread_sigmask(SIG_BLOCK, &sigpipe_mask, nullptr);
}

//当前更新当前线程的signal mask 并创建一个监测signals的signalfd
int CreateNoneBlockingSignalFd(std::initializer_list<int> signals) {
	int sfd, ret;
	sigset_t sigset;

  //get current sig mask set
  if (pthread_sigmask(SIG_SETMASK, NULL, &sigset) < 0) {
    return -1;
  }

  for (const int signal : signals) { //pending to set
	  ::sigaddset(&sigset, signal);
  }

	//::sigprocmask(SIG_SETMASK, &sigset, NULL);
	if (pthread_sigmask(SIG_SETMASK, &sigset, NULL) < 0) {
    return -1;
  }

	::sigemptyset(&sigset);
  for (const int signal : signals) {
	  ::sigaddset(&sigset, signal);
  }

	sfd = ::signalfd(-1, &sigset, SFD_NONBLOCK|SFD_CLOEXEC);
	if (sfd < 0)
    return -1;

	return sfd;
}

}
