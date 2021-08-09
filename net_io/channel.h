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

//
// Created by gh on 18-12-5.
//

#ifndef LIGHTINGIO_NET_CHANNEL_H
#define LIGHTINGIO_NET_CHANNEL_H

#include <base/base_constants.h>
#include <base/compiler_specific.h>
#include <cstdint>
#include "base/ip_endpoint.h"
#include "base/lt_micro.h"
#include "base/message_loop/fd_event.h"
#include "io_buffer.h"
#include "net_callback.h"

// socket chennel interface and base class
namespace base {
class EventPump;
}

namespace lt {
namespace net {

/*
 * 原始设计中，Channel是Owner FdEvent并管理链接状态的，并通过回调处理
 * 状态同步，因为回调层数过深，并存在错误处理状态的一些困难，后来经过
 * 思考后，将SocketChannel转化成BufferChannel + 驱动FdEvent侦听事件的
 * 简化版本,从而减少回调层数并可以被其他非回调式reactor模型使用：eg
 * coroutine + reactor 驱动的同步编程模式,  这样传统的reactor网络模型
 * 和基于coroutine的同步网络模型都可以复用了。
 *
 * IOWaiter iowaiter(fdev);
 * iowatier.Wait(timetout);
 * handleRead/Write
 * DataBuffer -> Codec -> Request/Response -> ServerHandler/ClientHandler
 * */
class SocketChannel {
public:
  enum class Status { CONNECTING, CONNECTED, CLOSING, CLOSED };

  virtual ~SocketChannel();

  void SetFdEvent(base::FdEvent* fdev) { fdev_ = fdev; };

  virtual bool StartChannel(bool server) WARN_UNUSED_RESULT;

  // read socket data to buffer
  // when success return true, else return false
  virtual bool HandleRead() WARN_UNUSED_RESULT = 0;

  // write as much as data into socket
  // return true when, false when error
  // handle err is responsibility of caller
  // the caller should decide close channel or not
  virtual bool TryFlush() WARN_UNUSED_RESULT = 0;

  /* return -1 when error, handle err is responsibility of caller
   * return 0 when all data pending to buffer,
   * other case return the bytes writen*/
  inline int32_t Send(const std::string& data) {
    return Send(data.data(), data.size());
  }

  /* return -1 when error, handle err is responsibility of caller
   * return 0 when all data pending to buffer,
   * other case return the bytes writen*/
  virtual int32_t Send(const char* data,
                       const int32_t len) WARN_UNUSED_RESULT = 0;

  IOBuffer* ReaderBuffer() { return &in_buffer_; }

  IOBuffer* WriterBuffer() { return &out_buffer_; }

  bool HasOutgoingData() const {return out_buffer_.CanReadSize();}  

  bool HasIncommingData() const {return in_buffer_.CanReadSize();}  

  Status GetStatus() const { return status_; }

  const std::string StatusStr() const;

  bool IsClosed() const { return status_ == Status::CLOSED; };

  bool IsConnected() const { return status_ == Status::CONNECTED; };

  bool IsConnecting() const { return status_ == Status::CONNECTING; };

  const IPEndPoint& LocalEndpoint() const { return local_ep_; }

  const IPEndPoint& RemoteEndPoint() const { return local_ep_; }

  std::string ChannelInfo() const;

protected:
  SocketChannel(int socket, const IPEndPoint& loc, const IPEndPoint& peer);

  void setup_channel();

  void close_channel();

  int32_t binded_fd() const { return fdev_ ? fdev_->GetFd() : -1; }

  std::string local_name() const;

  std::string remote_name() const;

  void SetChannelStatus(Status st);

protected:
  base::FdEvent* fdev_ = nullptr;

  IPEndPoint local_ep_;

  IPEndPoint remote_ep_;

  IOBuffer in_buffer_;

  IOBuffer out_buffer_;

private:
  Status status_ = Status::CONNECTING;

  DISALLOW_COPY_AND_ASSIGN(SocketChannel);
};

using SocketChannelPtr = std::unique_ptr<SocketChannel>;

}  // namespace net
}  // namespace lt
#endif  // LIGHTINGIO_NET_CHANNEL_H
