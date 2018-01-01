#ifndef NET_PROTOCOL_MESSAGE_H
#define NET_PROTOCOL_MESSAGE_H

#include <string>
#include <net_callback.h>

namespace net {

typedef enum {
  kUndefined = 0,
  kInRequest = 1,
  kInReponse = 2,
  kOutRequest = 3,
  kOutResponse = 4
} ProtoMsgType;

typedef struct {
  WeakPtrTcpChannel channel;
} IOContext;

typedef struct {
  void* data;
} WorkContext;

class ProtocolMessage {
public:
  ProtocolMessage(const std::string protocol);
  virtual ~ProtocolMessage();

  const std::string& Protocol() const;

  void SetMessageType(ProtoMsgType t);
  const ProtoMsgType MessageType() const;

  IOContext& GetIOCtx() {return io_context_;}
  WorkContext& GetWorkCtx() {return work_context_;}
  void SetIOContextWeakChannel(const RefTcpChannel& channel);
  void SetIOContextWeakChannel(WeakPtrTcpChannel& channel);

  void SetResponse(RefProtocolMessage& response);
  void SetResponse(RefProtocolMessage&& response);
  RefProtocolMessage Response() {return response_;}
protected:
  IOContext io_context_;
  WorkContext work_context_;
private:
  // Work Context

  std::string proto_;
  ProtoMsgType type_;
  RefProtocolMessage response_;
};

}
#endif
