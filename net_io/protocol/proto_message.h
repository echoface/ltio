#ifndef NET_PROTOCOL_MESSAGE_H
#define NET_PROTOCOL_MESSAGE_H

#include <string>
#include <net_callback.h>
#include <base/coroutine/coroutine.h>
#include <base/message_loop/message_loop.h>

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
} MessageCode;

typedef struct {
  base::MessageLoop* io_loop;
  WeakProtoService protocol_service;
} IOContext;

typedef enum {
  KNullId = 0,
} SequenceIdentify;

typedef struct {
  base::MessageLoop* loop;
  base::StlClosure resume_ctx;
} WorkContext;

class ProtocolMessage;
typedef std::weak_ptr<ProtocolMessage> WeakProtocolMessage;
typedef std::shared_ptr<ProtocolMessage> RefProtocolMessage;
typedef std::function<void(const RefProtocolMessage&)> ProtoMessageHandler;

#define RefCast(Type, RefObj) std::static_pointer_cast<Type>(RefObj)

class ProtocolMessage {
public:
  const static RefProtocolMessage kNullMessage;
  ProtocolMessage(MessageType t);
  virtual ~ProtocolMessage();

  const MessageType& GetMessageType() const {return type_;};
  bool IsRequest() const {return type_ == MessageType::kRequest;};

  void SetRemoteHost(const std::string& h) {remote_host_ = h;}
  const std::string& RemoteHost() const {return remote_host_;}

  IOContext& GetIOCtx() {return io_context_;}
  void SetIOCtx(const RefProtoService& service);

  WorkContext& GetWorkCtx() {return work_context_;}
  void SetWorkerCtx(base::MessageLoop* loop);
  void SetWorkerCtx(base::MessageLoop* loop, base::StlClosure coro_resumer);

  MessageCode FailCode() const;
  void SetFailCode(MessageCode reason);

  void SetResponse(const RefProtocolMessage& response);
  ProtocolMessage* RawResponse() {return response_.get();}
  const RefProtocolMessage& Response() const {return response_;}

  const char* TypeAsStr() const;
  bool IsResponded() const {return responded_;}

  /* use for those async-able protocol message,
   * use for matching request and response*/
  void SetAsyncIdentify(uint64_t id) {async_id_ = id;}
  const uint64_t AsyncIdentify() const {return async_id_;};

  virtual const std::string Dump() const {return "";};
protected:
  IOContext io_context_;
  WorkContext work_context_;
private:
  MessageType type_;
  std::string remote_host_;

  MessageCode fail_code_;
  bool responded_;
  RefProtocolMessage response_;
  uint64_t async_id_ = SequenceIdentify::KNullId;
};

}
#endif
