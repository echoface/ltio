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

#ifndef _LT_NET_CLIENT_CONNECTOR_H_H
#define _LT_NET_CLIENT_CONNECTOR_H_H

#include <sys/types.h>
#include <atomic>
#include <cstdint>
#include <list>

#include "net_io/codec/codec_factory.h"
#include "net_io/socket_acceptor.h"
#include "net_io/socket_utils.h"

namespace lt {
namespace net {

using base::FdEvent;
using base::EventPump;
using base::RefFdEvent;

typedef struct DialOption {
  int connect_timeout = -1; // in ms
} DialOption;

struct ConnDetail {
  int socket = 0;
  IPEndPoint local;
  IPEndPoint peer;

  bool valid() {return socket > 0 && local.address().IsValid() && peer.address().IsValid();}
};


class Connector : public base::FdEvent::Handler {
public:
  enum Error {
    kSocketErr = 1,
    kTimeout = 2,
  };
  class Delegate {
  public:
    virtual ~Delegate(){};

    // notify connect fail
    virtual void OnConnectFailed(uint32_t count) = 0;

    // replace OnConnectFailed
    virtual void OnConnectErr(Error err) {};

    virtual void OnConnected(int socket_fd,
                             IPEndPoint& local,
                             IPEndPoint& remote) = 0;
  };

  Connector(base::EventPump* pump);
  ~Connector(){};

  void Stop();

  void Dial(const net::IPEndPoint& ep, Delegate*);

  ConnDetail DialSync(const net::IPEndPoint& ep);

  uint32_t InprocessCount() const { return count_; }

private:
  typedef struct connect_ctx {
    RefFdEvent ev = nullptr;
    Delegate* hdl = nullptr;
    bool IsNil() const {return (!hdl || !ev);}
  } ctx;

  void HandleEvent(FdEvent* fdev, base::LtEv::Event ev) override;

  void HandleError(FdEvent* fd_event);

  void HandleWrite(FdEvent* fd_event);

  void RemoveFromInprogress(base::FdEvent* ev, bool notify_fail);

private:
  EventPump* pump_;

  std::atomic<uint32_t> count_;

  std::list<connect_ctx> inprogress_;
};

using RefConnector = std::shared_ptr<Connector>;
using OwnedConnector = std::unique_ptr<Connector>;

}  // namespace net
}  // namespace lt
#endif
