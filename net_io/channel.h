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

#include <cstdint>

#include "base/compiler_specific.h"
#include "base/ip_endpoint.h"
#include "base/logging.h"
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
 * Coroutine:
 * IOWaiter iowaiter(fdev);
 * iowatier.Wait(timetout);
 * Codec::HandleEvent(ev)
 *  Socket::HandleRead/Write
 *  Decode(buf) -cb-> Server/ClientHandler
 *  |
 *  | may async(none-io loop handler)
 *  |
 *  Encode(req/res)
 *  Socket::Write
 *
 * EventLoop:
 * Loop::WaitIO -cb->
 * FdEv::HandleEvent() -cb->
 * Codec::HandleEvent(ev)
 *  Socket::HandleRead/Write
 *  Decode(buf) -cb-> Server/ClientHandler
 *  |
 *  | may async(none-io loop handler)
 *  |
 *  Encode(req/res)
 *  Socket::Write
 *
 * DataFlow:
 * Socket|Buffer -> Codec -> Server/ClientHandler -|-> Codec -> Socket|Buffer
 *
 * */

class SocketChannel {
public:
  virtual ~SocketChannel();

  void SetFdEvent(base::FdEvent* fdev) { fdev_ = fdev; };

  virtual bool StartChannel(bool server) WARN_UNUSED_RESULT;

  /*
  * read socket as much as to in_buf
  * On success, return the nbytes in in_buf returned
  * return <0 when socket error or other fatal error
  * the caller should decide close channel or not
  */
  virtual int HandleRead() WARN_UNUSED_RESULT = 0;

  /*
   * write as much as data into socket
   * on success, return the nbytes write to peer
   * return <0 when socket error or other fatal error
   * the caller should decide close channel or not
   */
  virtual int HandleWrite() WARN_UNUSED_RESULT = 0;

  /*
   * return 0 when all data pending to out buffer,
   * other case return nbytes realy write to socket
   * return -1 when error, err-handle is responsibility of caller
   * */
  inline int32_t Send(const std::string& data) WARN_UNUSED_RESULT {
    return Send(data.data(), data.size());
  }

  /*
   * return 0 when all data pending to out buffer,
   * other case return nbytes realy write to socket,
   * remain bytes (len - nbytes) pending to out buf,
   * return -1 when error, err-handle is responsibility of caller
   * */
  virtual int32_t Send(const char* data,
                       const int32_t len) WARN_UNUSED_RESULT = 0;

  IOBuffer* ReaderBuffer() { return &in_; }

  IOBuffer* WriterBuffer() { return &out_; }

  bool HasOutgoingData() const { return out_.CanReadSize(); }

  bool HasIncommingData() const { return in_.CanReadSize(); }

  const IPEndPoint& LocalEndpoint() const { return local_ep_; }

  const IPEndPoint& RemoteEndPoint() const { return local_ep_; }

  std::string ChannelInfo() const;

protected:
  SocketChannel(int socket, const IPEndPoint& loc, const IPEndPoint& peer);

  int32_t binded_fd() const { return fdev_ ? fdev_->GetFd() : -1; }

  std::string local_name() const;

  std::string remote_name() const;

protected:
  // not own fdev
  base::FdEvent* fdev_ = nullptr;

  IPEndPoint local_ep_;

  IPEndPoint remote_ep_;

  IOBuffer in_;

  IOBuffer out_;

private:
  DISALLOW_COPY_AND_ASSIGN(SocketChannel);
};

using SocketChannelPtr = std::unique_ptr<SocketChannel>;

}  // namespace net
}  // namespace lt
#endif  // LIGHTINGIO_NET_CHANNEL_H
