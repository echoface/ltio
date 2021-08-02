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

#ifndef _NET_SERVICE_ACCEPTOR_H_H_
#define _NET_SERVICE_ACCEPTOR_H_H_

#include "base/ip_endpoint.h"
#include "socket_utils.h"

#include "base/lt_micro.h"
#include "base/message_loop/fd_event.h"
#include "base/message_loop/message_loop.h"
#include "net_callback.h"

namespace lt {
namespace net {

class SocketAcceptor : public base::FdEvent::Handler {
public:
  class Actor {
  public:
    virtual bool AutoRestart() {return true;}
    virtual void OnFatalError() {CHECK(false);}
    virtual void OnNewConnection(int /*fd*/, const IPEndPoint&) = 0;
  };
public:
  SocketAcceptor(Actor*, base::EventPump* pump, const IPEndPoint&);
  ~SocketAcceptor();

  bool StartListen();

  void StopListen();

  bool IsListening() { return listening_; }

  const IPEndPoint& ListeningAddress() const { return address_; };
private:
  bool InitListener();
  // override from FdEvent::Handler
  void HandleEvent(base::FdEvent* fdev);

  void AcceptRead(base::FdEvent* fdev);
  bool HandleError(base::FdEvent* fd_event);

  bool listening_;
  IPEndPoint address_;

  Actor* handler_;
  base::EventPump* event_pump_;
  base::RefFdEvent socket_event_;
  DISALLOW_COPY_AND_ASSIGN(SocketAcceptor);
};

}  // namespace net
}  // namespace lt
#endif
// end of file
