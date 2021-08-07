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
#include "base/base_constants.h"
#include "base/closure/closure_task.h"
#include "base/message_loop/event_pump.h"
#include "base/utils/sys_error.h"
#include "glog/logging.h"
#include "net_callback.h"
#include "socket_utils.h"

namespace {
const int32_t kBlockSize = 2 * 1024;

// return true when success, false when error hanpened
bool ReadAllFromSocket(base::FdEvent* fdev, lt::net::IOBuffer* buf) {
  int socket = fdev->GetFd();
  ssize_t bytes_read = 0;
  do {
    buf->EnsureWritableSize(kBlockSize);
    bytes_read = ::read(socket, buf->GetWrite(), buf->CanWriteSize());
    if (bytes_read > 0) {
      buf->Produce(bytes_read);
      continue;
    }
    if (errno == EINTR) {
      continue;
    }
    VLOG_IF(GLOG_VTRACE, bytes_read == 0) << "peer closed, fd:" << socket;
    break;
  } while (true);
  return errno == EAGAIN ? true : false;
}

// return true when success, false when fail
bool WriteAllToSocket(base::FdEvent* fdev, lt::net::IOBuffer* buf) {
  int socket_fd = fdev->GetFd();
  ssize_t writen_bytes = 0;
  bool success = true;
  while (buf->CanReadSize()) {
    writen_bytes = ::write(socket_fd, buf->GetRead(), buf->CanReadSize());
    if (writen_bytes > 0) {
      buf->Consume(writen_bytes);
      continue;
    }
    if (errno == EINTR) {
      continue;
    }
    success = (errno == EAGAIN ? true : false);
    break;
  };

  if (buf->CanReadSize()) {
    fdev->EnableWriting();
  } else {
    fdev->DisableWriting();
  }
  return success;
}

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
  : SocketChannel(socket_fd, loc, peer) {
  fd_event_->SetEdgeTrigger(true);
  socketutils::TCPNoDelay(socket_fd);
  socketutils::KeepAlive(socket_fd, true);
}

TcpChannel::~TcpChannel() {
  VLOG(GLOG_VTRACE) << __FUNCTION__ << ChannelInfo();
}

void TcpChannel::HandleEvent(base::FdEvent* fdev) {
  if (out_buffer_.CanReadSize()) {
    if (!WriteAllToSocket(fdev, &out_buffer_)) {
      goto err_handle;
    }
    if (out_buffer_.CanReadSize()) {
      reciever_->OnDataFinishSend(this);
    }
  }
  if (fdev->ReadFired()) {
    if (!ReadAllFromSocket(fdev, &in_buffer_)) {
      goto err_handle;
    }
  }
  if (in_buffer_.CanReadSize()) {
    reciever_->OnDataReceived(this, &in_buffer_);
  }
  if (!schedule_shutdown_) {
    return;
  }
err_handle:
  HandleClose(fdev);
  return;
}

bool TcpChannel::TryFlush() {
  if (out_buffer_.CanReadSize()) {
    bool success = WriteAllToSocket(fd_event_.get(), &out_buffer_);
    return success;
  }
  return true;
}

int32_t TcpChannel::Send(const char* data, const int32_t len) {
  DCHECK(pump_->IsInLoop());

  if (!IsConnected()) {
    return -1;
  }

  if (out_buffer_.CanReadSize() > 0) {
    out_buffer_.WriteRawData(data, len);
    WriteAllToSocket(fd_event_.get(), &out_buffer_);
    return len;
  }

  int32_t fatal_err = 0;
  size_t n_write = 0;
  size_t n_remain = len;
  do {
    ssize_t part_write =
        socketutils::Write(fd_event_->GetFd(), data + n_write, n_remain);
    if (part_write > 0) {
      n_write += part_write;
      n_remain = n_remain - part_write;

      DCHECK((n_write + n_remain) == size_t(len));
      continue;
    }

    if (errno == EAGAIN) {
      out_buffer_.WriteRawData(data + n_write, n_remain);
    } else if (errno == EINTR) {
      continue;
    } else {
      // to avoid A -> B -> callback delete A problem,
      // use a write event to triggle handle close action
      fatal_err = errno;
      schedule_shutdown_ = true;
      SetChannelStatus(Status::CLOSING);
      LOG(ERROR) << ChannelInfo() << " send err:" << base::StrError()
                 << " schedule close";
    }
    break;
  } while (n_remain != 0);

  if (out_buffer_.CanReadSize()) {
    fd_event_->EnableWriting();
  }
  return fatal_err != 0 ? -1 : n_write;
}

}  // namespace net
}  // namespace lt
