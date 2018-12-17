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
typedef std::shared_ptr<ProtocolMessage> RefProtocolMessage;
typedef std::weak_ptr<ProtocolMessage> WeakProtocolMessage;
typedef std::function<void(const RefProtocolMessage&/*request*/)> ProtoMessageHandler;

class ProtocolMessage {
public:
  const static RefProtocolMessage kNullMessage;
  ProtocolMessage(const std::string, MessageType t);
  virtual ~ProtocolMessage();

  const std::string& Protocol() const;
  const MessageType& GetMessageType() const {return type_;};
  bool IsRequest() const {return type_ == MessageType::kRequest;};

  IOContext& GetIOCtx() {return io_context_;}
  WorkContext& GetWorkCtx() {return work_context_;}
  void SetIOContext(const RefProtoService& service);

  MessageCode FailCode() const;
  void SetFailCode(MessageCode reason);
  void AppendFailMessage(std::string m);
  const std::string& FailMessage() const;

  void SetResponse(const RefProtocolMessage& response);
  void SetResponse(RefProtocolMessage&& response);
  ProtocolMessage* RawResponse() {return response_.get();}
  const RefProtocolMessage& Response() const {return response_;}

  const char* TypeAsStr() const;
  bool IsResponded() const {return responsed_;}
  virtual const std::string Dump() const {return "";};
  virtual const uint64_t Identify() const {return SequenceIdentify::KNullId;};
protected:
  IOContext io_context_;
  WorkContext work_context_;
private:
  // Work Context
  MessageType type_;
  std::string proto_;

  MessageCode fail_code_;
  std::string fail_;
  bool responsed_;
  RefProtocolMessage response_;
};

}
#endif
