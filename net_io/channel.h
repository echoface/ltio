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
#include "io_buffer.h"
#include "net_callback.h"
#include "base/base_micro.h"
#include <base/base_constants.h>
#include "base/message_loop/event_pump.h"
#include "base/message_loop/message_loop.h"

#include "base/ip_endpoint.h"

// socket chennel interface and base class
namespace lt {
namespace net {


class SocketChannel : public base::FdEvent::Handler {
public:
  enum class Status {
    CONNECTING,
    CONNECTED,
	  CLOSING,
    CLOSED
  };
  typedef struct Reciever {
    virtual void OnChannelReady(const SocketChannel*) {};
    virtual void OnDataFinishSend(const SocketChannel*) {};
    virtual void OnChannelClosed(const SocketChannel*) = 0;
    virtual void OnDataReceived(const SocketChannel*, IOBuffer *) = 0;
  }Reciever;

public:
  void StartChannel();
  void SetReciever(SocketChannel::Reciever* consumer);

  virtual bool TryFlush();
  /* return -1 when error,
   * return 0 when all data pending to buffer,
   * other case return the byte of writen*/
  virtual int32_t Send(const char* data, const int32_t len) = 0;

  /* a initiative call from application level to clase this channel
   * 1. shutdown writer if writing is enable
   * 2. directly unregiste fdevent from pump and close socket*/
  virtual void ShutdownChannel(bool half_close) = 0;

  bool IsConnected() const {return status_ == Status::CONNECTED;};
  IOBuffer* WriterBuffer() {return &out_buffer_;}

  std::string ChannelInfo() const;
  const std::string StatusAsString() const;
  const std::string& ChannelName() const {return name_;}
protected:
  SocketChannel(int socket_fd,
                const IPEndPoint& loc,
                const IPEndPoint& peer,
                base::EventPump* pump);
  virtual ~SocketChannel() {
    CHECK(status_ == Status::CLOSED);
  }

  void setup_channel();
  void close_channel();
  int32_t binded_fd() const;
  std::string local_name() const;
  std::string remote_name() const;

  void SetChannelStatus(Status st);

  /* channel-relatived pump for event notify */
  base::EventPump* pump_;

  bool schedule_shutdown_ = false;

  base::RefFdEvent fd_event_;

  IPEndPoint local_ep_;
  IPEndPoint remote_ep_;

  std::string name_;

  IOBuffer in_buffer_;
  IOBuffer out_buffer_;

  SocketChannel::Reciever* reciever_ = NULL;
private:
  Status status_ = Status::CONNECTING;

  DISALLOW_COPY_AND_ASSIGN(SocketChannel);
};

}} //lt::net
#endif //LIGHTINGIO_NET_CHANNEL_H
