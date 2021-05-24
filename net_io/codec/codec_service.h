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

#ifndef _NET_PROTOCOL_SERVICE_H_H
#define _NET_PROTOCOL_SERVICE_H_H

#include "codec_message.h"

#include <net_io/url_utils.h>
#include <net_io/net_callback.h>
#include <net_io/tcp_channel.h>
#include <net_io/base/ip_endpoint.h>
#include <base/message_loop/message_loop.h>

namespace base {
  class MessageLoop;
}

namespace lt {
namespace net {

/* a stateless encoder/decoder and
 * transfer the ProtoMessage to real Handler */
class CodecService : public EnableShared(CodecService),
                     public SocketChannel::Reciever {
public:
  class Delegate {
    public:
      virtual void OnCodecMessage(const RefCodecMessage& message) = 0;
      virtual void OnProtocolServiceReady(const RefCodecService& service) {};
      virtual void OnProtocolServiceGone(const RefCodecService& service) = 0;
      //for client side
      virtual const url::RemoteInfo* GetRemoteInfo() const {return NULL;};
  };

public:
  CodecService(base::MessageLoop* loop);
  virtual ~CodecService();

  void SetDelegate(Delegate* d);
  /* this can be override for create diffrent type SocketChannel,
   * eg SSLChannel, UdpChannel, ....... */
  virtual bool BindToSocket(int fd,
                            const IPEndPoint& local,
                            const IPEndPoint& peer);

  virtual void StartProtocolService();

  TcpChannel* Channel() {return channel_.get();};
  base::MessageLoop* IOLoop() const { return binded_loop_;}
  base::EventPump* Pump() const { return binded_loop_->Pump();}

  void CloseService(bool block_callback = false);
  bool IsConnected() const;

  virtual void BeforeCloseService() {};
  virtual void AfterChannelClosed() {};

  /* feature indentify*/
  //async clients request
  virtual bool KeepSequence() {return true;};
  virtual bool KeepHeartBeat() {return false;}

  virtual bool EncodeToChannel(CodecMessage* message) = 0;
  virtual bool EncodeResponseToChannel(const CodecMessage* req, CodecMessage* res) = 0;

  virtual const RefCodecMessage NewHeartbeat() {return NULL;}
  virtual const RefCodecMessage NewResponse(const CodecMessage*) {return NULL;}

  virtual const std::string& protocol() const {
    const static std::string kEmpty;
    return kEmpty;
  };

  void SetIsServerSide(bool server_side);
  bool IsServerSide() const {return server_side_;}
  inline MessageType InComingType() const {
    return server_side_ ? MessageType::kRequest : MessageType::kResponse;
  }
protected:
  // override this do initializing for client side, like set db, auth etc
  virtual void OnChannelReady(const SocketChannel*) override;

  void OnChannelClosed(const SocketChannel*) override;

  bool server_side_;
  TcpChannelPtr channel_;
  Delegate* delegate_ = nullptr;
  base::MessageLoop* binded_loop_ = nullptr;
  DISALLOW_COPY_AND_ASSIGN(CodecService);
};

}}
#endif
