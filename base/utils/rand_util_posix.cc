// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <errno.h>
#include <fcntl.h>
#include <stddef.h>
#include <stdint.h>
#include <unistd.h>

#include "glog/logging.h"

#include "no_destructor.h"
#include "rand_util.h"

namespace {

#include <errno.h>

#if defined(NDEBUG)

#define HANDLE_EINTR(x) ({ \
  decltype(x) eintr_wrapper_result; \
  do { \
    eintr_wrapper_result = (x); \
  } while (eintr_wrapper_result == -1 && errno == EINTR); \
  eintr_wrapper_result; \
})

#else

#define HANDLE_EINTR(x) ({ \
  int eintr_wrapper_counter = 0; \
  decltype(x) eintr_wrapper_result; \
  do { \
    eintr_wrapper_result = (x); \
  } while (eintr_wrapper_result == -1 && errno == EINTR && \
           eintr_wrapper_counter++ < 100); \
  eintr_wrapper_result; \
})

#endif  // NDEBUG

// We keep the file descriptor for /dev/urandom around so we don't need to
// reopen it (which is expensive), and since we may not even be able to reopen
// it if we are later put in a sandbox. This class wraps the file descriptor so
// we can use a static-local variable to handle opening it on the first access.
class URandomFd {
 public:
#if defined(OS_AIX)
  // AIX has no 64-bit support for open falgs such as -
  //  O_CLOEXEC, O_NOFOLLOW and O_TTY_INIT
  URandomFd() : fd_(HANDLE_EINTR(open("/dev/urandom", O_RDONLY))) {
    LOG_IF(ERROR, fd_ < 0) << "Cannot open /dev/urandom";
  }
#else
  URandomFd() : fd_(HANDLE_EINTR(open("/dev/urandom", O_RDONLY | O_CLOEXEC))) {
    LOG_IF(ERROR, fd_ < 0) << "Cannot open /dev/urandom";
  }
#endif

  ~URandomFd() { close(fd_); }

  int fd() const { return fd_; }

 private:
  const int fd_;
};

}  // namespace

namespace base {

bool ReadFromFD(int fd, char* buffer, size_t bytes) {
  size_t total_read = 0;
  while (total_read < bytes) {
    ssize_t bytes_read =
        HANDLE_EINTR(read(fd, buffer + total_read, bytes - total_read));
    if (bytes_read <= 0)
      break;
    total_read += bytes_read;
  }
  return total_read == bytes;
}

void RandBytes(void* output, size_t output_length) {
  const int urandom_fd = GetUrandomFD();
  const bool success =
      ReadFromFD(urandom_fd, static_cast<char*>(output), output_length);
  CHECK(success);
}

int GetUrandomFD() {
  static NoDestructor<URandomFd> urandom_fd;
  return urandom_fd->fd();
}

}  // namespace base
