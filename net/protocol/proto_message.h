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
  kNothing = 0,
  kTimeOut = 1,
  kChannelBroken = 2,
  kBadMessage = 3,
} FailInfo;

typedef struct {
  WeakProtoService protocol_service;
} IOContext;

typedef enum {
  KInvalidIdentify = 0,
} MessageIdentifyType;

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
  ProtocolMessage(const std::string, MessageType t);
  virtual ~ProtocolMessage();

  const std::string& Protocol() const;
  const MessageType& GetMessageType() const {return type_;};
  bool IsRequestMessage() const {return type_ == MessageType::kRequest;};

  IOContext& GetIOCtx() {return io_context_;}
  WorkContext& GetWorkCtx() {return work_context_;}
  void SetIOContext(const RefProtoService& service);

  void SetFailInfo(FailInfo reason);
  FailInfo MessageFailInfo() const;
  const std::string& FailMessage() const;
  void AppendFailMessage(std::string m);

  void SetResponse(const RefProtocolMessage& response);
  void SetResponse(RefProtocolMessage&& response);
  ProtocolMessage* RawResponse() {return response_.get();}
  const RefProtocolMessage& Response() const {return response_;}

  const char* MessageTypeStr() const;
  bool IsResponsed() const {return responsed_;}
  virtual const uint64_t MessageIdentify() const {return 0;};
  virtual const std::string MessageDebug() const {return "";};
protected:
  IOContext io_context_;
  WorkContext work_context_;
private:
  // Work Context
  MessageType type_;
  std::string proto_;

  FailInfo fail_info_;
  std::string fail_;
  bool responsed_;
  RefProtocolMessage response_;
};

}
#endif
