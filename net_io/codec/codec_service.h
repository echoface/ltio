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

#include "base/lt_macro.h"
#include "base/message_loop/message_loop.h"
#include "net_io/common/ip_endpoint.h"
#include "net_io/channel.h"
#include "net_io/net_callback.h"
#include "net_io/url_utils.h"

#include "codec_message.h"

namespace base {
class MessageLoop;
}

namespace lt {
namespace net {

/* as name mean, decode bytestream to a specific message from channel
 * and encode request/response to bytestream and write to channel
 * */
class CodecService : public EnableShared(CodecService),
                     public base::FdEvent::Handler {
public:
  enum class Status { CONNECTING, CONNECTED, CLOSING, CLOSED };

  class Handler {
  public:
    virtual void OnCodecMessage(const RefCodecMessage& message) = 0;
  };

  class Delegate {
  public:
    virtual void OnCodecReady(const RefCodecService& service){};

    virtual void OnCodecClosed(const RefCodecService& service) = 0;

    // for client side
    virtual const url::RemoteInfo* GetRemoteInfo() const { return NULL; };
  };

public:
  CodecService(base::MessageLoop* loop);

  virtual ~CodecService();

  void SetDelegate(Delegate* d) { delegate_ = d; }

  Handler* GetHandler() { return handler_; }

  void SetHandler(Handler* handler) { handler_ = handler; }

  void SetProtocol(const std::string& protocol) { protocol_ = protocol; };

  void SetAsFdEvHandler(bool as_handler) { as_fdev_handler_ = as_handler; }

  void BindSocket(base::RefFdEvent fdev, SocketChannelPtr&& channel);

  virtual void StartProtocolService();

  // just close codec service, call notify if needed
  void CloseService(bool block_callback = false);

  SocketChannel* Channel() { return channel_.get(); };

  base::MessageLoop* IOLoop() const { return loop_; }

  base::EventPump* Pump() const { return loop_->Pump(); };

  bool ShouldClose() const;

  bool IsClosed() const { return status_ == Status::CLOSED; }

  bool IsConnected() const { return status_ == Status::CONNECTED; }

  virtual void OnDataReceived(IOBuffer* buffer) = 0;

  virtual void BeforeCloseService(){};

  virtual void AfterChannelClosed(){};

  /* feature indentify async clients request*/
  virtual bool KeepSequence() { return true; };

  virtual bool KeepHeartBeat() { return false; }

  virtual bool SendRequest(CodecMessage* message) WARN_UNUSED_RESULT = 0;

  virtual bool SendResponse(const CodecMessage* req,
                            CodecMessage* res) WARN_UNUSED_RESULT = 0;

  virtual const RefCodecMessage NewHeartbeat() WARN_UNUSED_RESULT {
    return NULL;
  }

  virtual const RefCodecMessage NewResponse(const CodecMessage*)
      WARN_UNUSED_RESULT {
    return NULL;
  }

  virtual bool UseSSLChannel() const { return false; };

  const std::string& protocol() const { return protocol_; };

  void SetIsServerSide(bool server_side);

  bool IsServerSide() const { return server_side_; }

  // the event drive api, call from pumped fdevent
  // or
  // other delegate event driver, eg: co::IOEvent
  void HandleEvent(base::FdEvent* fdev, base::LtEv::Event ev) override;

  // override this for some need after-send-action codec
  virtual void OnDataFinishSend();

protected:
  void StartInternal();

  bool TryFlushChannel();

  // true for success, false when fatal error
  bool SocketReadWrite(base::LtEv::Event ev);

  // notify delegate_ codec is ready
  void NotifyCodecReady();

  // will trigger AfterChannelClosed and notify to delegate_
  void NotifyCodecClosed();

  bool server_side_;

  std::string protocol_;

  // ScheduleClose will apply close action for a
  // connected codec/channel when data finish send
  bool schedule_close_ = false;

  base::RefFdEvent fdev_;

  // case 1: as fdevent handler, drive by io-pump
  // case 2: drive by top level controller, eg: co::IOEvent
  bool as_fdev_handler_ = true;

  SocketChannelPtr channel_;

  Handler* handler_ = nullptr;

  Delegate* delegate_ = nullptr;

  base::MessageLoop* loop_ = nullptr;

  Status status_ = Status::CONNECTING;

  DISALLOW_COPY_AND_ASSIGN(CodecService);
};

}  // namespace net
}  // namespace lt
#endif
