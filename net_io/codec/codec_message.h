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

#ifndef NET_PROTOCOL_MESSAGE_H
#define NET_PROTOCOL_MESSAGE_H

#include <base/closure/closure_task.h>
#include <net_io/net_callback.h>
#include <string>

namespace base {
class MessageLoop;
}

namespace lt {
namespace net {

enum class MessageType {
  kRequest,
  kResponse,
};

typedef enum {
  kSuccess = 0,
  kTimeOut = 1,
  kConnBroken = 2,
  kBadMessage = 3,
  kNotConnected = 4,
} MessageCode;

typedef struct {
  base::MessageLoop* io_loop;
  WeakCodecService codec;
} IOContext;

typedef struct {
  base::MessageLoop* loop;
  base::LtClosure resumer_fn;
} WorkContext;

class CodecMessage;
typedef std::weak_ptr<CodecMessage> WeakCodecMessage;
typedef std::shared_ptr<CodecMessage> RefCodecMessage;
typedef std::function<void(const RefCodecMessage&)> ProtoMessageHandler;

#define RefCast(Type, RefObj) std::static_pointer_cast<Type>(RefObj)

class CodecMessage {
public:
  CodecMessage();

  virtual ~CodecMessage();

  const IOContext& GetIOCtx() const { return io_context_; }

  void SetIOCtx(const RefCodecService& service);

  WorkContext& GetWorkCtx() { return work_context_; }

  void SetWorkerCtx(base::MessageLoop* loop);

  void SetWorkerCtx(base::MessageLoop* loop, base::LtClosure resumer);

  void SetTimeout();

  void SetFailCode(MessageCode reason);

  MessageCode FailCode() const;

  void SetResponse(const RefCodecMessage& response);

  CodecMessage* RawResponse() { return response_.get(); }

  const RefCodecMessage& Response() const { return response_; }

  /* use for those async-able protocol message,
   * use for matching request and response*/
  virtual bool AsHeartbeat() { return false; };

  virtual bool IsHeartbeat() const { return false; };

  virtual void SetAsyncId(uint64_t id){};

  virtual const uint64_t AsyncId() const { return 0; };

  // message for dubuging/testing purpose
  virtual const std::string Dump() const { return ""; };

  const static RefCodecMessage kNullMessage;

protected:
  IOContext io_context_;

  WorkContext work_context_;

private:
  MessageCode code_;

  RefCodecMessage response_;
};

}  // namespace net
}  // namespace lt
#endif
