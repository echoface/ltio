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

#ifndef NET_TCP_CHANNAL_CONNECTION_H_
#define NET_TCP_CHANNAL_CONNECTION_H_

#include <memory>
#include <functional>

#include "base/message_loop/event_pump.h"
#include "channel.h"
#include "net_callback.h"
#include "codec/codec_message.h"
/* *
 * all of this thing happend in io-loop its attached, include callbacks
 * */
namespace lt {
namespace net {

class TcpChannel : public SocketChannel {
public:
  static TcpChannelPtr Create(int socket_fd,
                              const IPEndPoint& local,
                              const IPEndPoint& peer,
                              base::EventPump* pump);
  ~TcpChannel();

  void ShutdownChannel(bool half_close) override;
  void ShutdownWithoutNotify();

  int32_t Send(const char* data, const int32_t len) override;
protected:
  TcpChannel(int socket_fd,
             const IPEndPoint& loc,
             const IPEndPoint& peer,
             base::EventPump* pump);

  bool HandleWrite(base::FdEvent* event) override;
  bool HandleRead(base::FdEvent* event) override;
  bool HandleError(base::FdEvent* event) override;
  bool HandleClose(base::FdEvent* event) override;

private:
  DISALLOW_COPY_AND_ASSIGN(TcpChannel);
};

}}
#endif
