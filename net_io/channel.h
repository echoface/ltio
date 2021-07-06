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
#include <cstdint>
#include "base/base_micro.h"
#include "base/ip_endpoint.h"
#include "base/message_loop/fd_event.h"
#include "io_buffer.h"
#include "net_callback.h"

// socket chennel interface and base class
namespace base {
class EventPump;
}

namespace lt {
namespace net {

class SocketChannel : public base::FdEvent::Handler {
public:
  enum class Status { CONNECTING, CONNECTED, CLOSING, CLOSED };
  typedef struct Reciever {
    virtual void OnChannelReady(const SocketChannel*){};
    virtual void OnDataFinishSend(const SocketChannel*){};
    virtual void OnChannelClosed(const SocketChannel*) = 0;
    virtual void OnDataReceived(const SocketChannel*, IOBuffer*) = 0;

    // delegate interface
    virtual bool IsServerSide() const = 0;
  } Reciever;

public:
  virtual ~SocketChannel();

  virtual void StartChannel();

  void SetIOEventPump(base::EventPump* pump);

  void SetReciever(SocketChannel::Reciever* consumer);

  virtual bool TryFlush();

  /* return -1 when error,
   * return 0 when all data pending to buffer,
   * other case return the byte of writen*/
  virtual int32_t Send(const char* data, const int32_t len) = 0;

  /* a initiative call from codec to close channel
   * 1. shutdown writer if writing is enable
   * 2. unregiste fdevent from pump and close socket*/
  virtual void ShutdownChannel(bool half_close);

  virtual void ShutdownWithoutNotify();

  IOBuffer* WriterBuffer() { return &out_buffer_; }

  Status GetStatus() const {return status_;}
  bool IsClosed() const { return status_ == Status::CLOSED; };
  bool IsConnected() const { return status_ == Status::CONNECTED; };
  bool IsConnecting() const { return status_ == Status::CONNECTING; };

  std::string ChannelInfo() const;

  const std::string StatusAsString() const;

  const std::string& ChannelName() const { return name_; }

protected:
  SocketChannel(int socket_fd,
                const IPEndPoint& loc,
                const IPEndPoint& peer);

  void setup_channel();

  void close_channel();

  int32_t binded_fd() const;

  std::string local_name() const;

  std::string remote_name() const;

  void SetChannelStatus(Status st);

  virtual bool HandleError(base::FdEvent* event) override;

  virtual bool HandleClose(base::FdEvent* event) override;

protected:
  std::string name_;

  base::EventPump* pump_;

  base::RefFdEvent fd_event_;

  IPEndPoint local_ep_;
  IPEndPoint remote_ep_;

  IOBuffer in_buffer_;
  IOBuffer out_buffer_;

  bool schedule_shutdown_ = false;

  SocketChannel::Reciever* reciever_ = NULL;

private:
  Status status_ = Status::CONNECTING;

  DISALLOW_COPY_AND_ASSIGN(SocketChannel);
};
using SocketChannelPtr = std::unique_ptr<SocketChannel>;

}  // namespace net
}  // namespace lt
#endif  // LIGHTINGIO_NET_CHANNEL_H
