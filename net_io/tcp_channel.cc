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
#include "tcp_channel.h"

#include <cmath>

#include "base/logging.h"
#include "base/utils/sys_error.h"
#include "net_callback.h"
#include "socket_utils.h"

#include "glog/logging.h"

namespace {
// support dynamic buffer size sugguest from codec
constexpr int32_t kBlockSize = 2 * 1024;
}  // namespace

namespace lt {
namespace net {

// static
TCPChannelPtr TcpChannel::Create(int socket_fd,
                                 const IPEndPoint& local,
                                 const IPEndPoint& peer) {
  return TCPChannelPtr(new TcpChannel(socket_fd, local, peer));
}

TcpChannel::TcpChannel(int socket_fd,
                       const IPEndPoint& loc,
                       const IPEndPoint& peer)
  : SocketChannel(socket_fd, loc, peer) {}

TcpChannel::~TcpChannel() {
  VLOG(VTRACE) << __FUNCTION__ << ChannelInfo();
}

int TcpChannel::HandleRead() {
  int socket = fdev_->GetFd();
  ssize_t bytes_read = 0;
  do {
    in_.EnsureWritableSize(kBlockSize);
    bytes_read = ::read(socket, in_.GetWrite(), in_.CanWriteSize());
    if (bytes_read > 0) {
      in_.Produce(bytes_read);
      VLOG(VTRACE) << ChannelInfo() << ", read:" << bytes_read
                   << ", buflen:" << in_.CanReadSize();
      continue;
    }
    if (bytes_read == 0) {
      VLOG(VTRACE) << ChannelInfo() << ", read info:" << base::StrError();
      return -1;
    }
    if (errno == EAGAIN) {
      break;
    }
    if (errno == EINTR) {
      VLOG(VTRACE) << ChannelInfo() << ", read info:" << base::StrError();
      continue;
    }
    // fatal error handle
    LOG(ERROR) << ChannelInfo() << ", read err:" << base::StrError();
    return -1;
  } while (true);

  return in_.CanReadSize();
}

int TcpChannel::HandleWrite() {
  // raw tcp channel, nothing should be done when empty
  if (out_.CanReadSize() == 0) {
    return 0;
  }

  int fd = fdev_->GetFd();
  ssize_t total_write = 0;

  while (out_.CanReadSize()) {
    ssize_t rv = write(fd, out_.GetRead(), out_.CanReadSize());
    if (rv > 0) {
      total_write += rv;
      out_.Consume(rv);
      VLOG(VTRACE) << ChannelInfo() << ", write:" << rv;
      continue;
    }
    if (errno == EAGAIN) {
      break;
    }
    if (errno == EINTR) {
      VLOG(VTRACE) << ChannelInfo() << ", write info:" << base::StrError();
      continue;
    }
    LOG(ERROR) << ChannelInfo() << ", write err:" << base::StrError();
    return -1;
  };

  out_.CanReadSize() ? fdev_->EnableWriting() : fdev_->DisableWriting();
  return total_write;
}

int32_t TcpChannel::Send(const char* data, const int32_t len) {
  if (out_.CanReadSize() > 0) {
    out_.WriteRawData(data, len);
    return HandleWrite();
  }

  size_t n_write = 0;
  size_t n_remain = len;
  do {
    ssize_t rv = ::write(fdev_->GetFd(), data + n_write, n_remain);
    if (rv > 0) {
      n_write += rv;
      n_remain = n_remain - rv;
      DCHECK((n_write + n_remain) == size_t(len));
      VLOG(VTRACE) << ChannelInfo() << ", write:" << rv;
      continue;
    }
    if (errno == EAGAIN) {  // most likely case
      fdev_->EnableWriting();
      out_.WriteRawData(data + n_write, n_remain);
      return n_write;
    }
    if (errno == EINTR) {
      continue;
    }

    LOG(ERROR) << ChannelInfo() << ", write err:" << base::StrError();
    return -1;
  } while (n_remain != 0);

  return n_write;
}

}  // namespace net
}  // namespace lt
