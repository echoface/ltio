#ifndef _NET_PROTOCOL_SERVICE_H_H
#define _NET_PROTOCOL_SERVICE_H_H

#include "../net_callback.h"
#include "proto_message.h"

namespace net {

class ChannelWriter;

enum class ProtocolServiceType{
  kServer,
  kClient
};

/* a stateless encoder/decoder and
 * transfer the ProtoMessage to real Handler */
class ProtoService {
public:
  ProtoService();
  virtual ~ProtoService();

  void SetMessageHandler(ProtoMessageHandler);
  void SetChannelWriter(ChannelWriter* writer);

  virtual void OnStatusChanged(const RefTcpChannel&) = 0;
  virtual void OnDataFinishSend(const RefTcpChannel&) = 0;

  virtual void OnDataRecieved(const RefTcpChannel&, IOBuffer*) = 0;
  virtual bool EncodeToBuffer(const ProtocolMessage* msg, IOBuffer* out_buffer) = 0;
  //Before send [request type] message, in normal case, this was used for
  //async clients request
  virtual bool KeepSequence() {return true;};
  virtual bool BeforeSendRequest(ProtocolMessage *message) {return true;};
  virtual void BeforeSendResponse(ProtocolMessage *in, ProtocolMessage *out) {};

  virtual bool SendProtocolMessage(RefProtocolMessage& message) = 0;

  /* io level notify. last chance to modify the message send to peer*/
  void BeforeWriteMessage(ProtocolMessage* message);
  virtual void BeforeWriteRequestToBuffer(ProtocolMessage* request) {};
  virtual void BeforeWriteResponseToBuffer(ProtocolMessage* response) {};

  virtual bool CloseAfterMessage(ProtocolMessage*, ProtocolMessage*) { return true;};
  virtual const RefProtocolMessage NewResponseFromRequest(const RefProtocolMessage &) {return NULL;}

  void SetServiceType(ProtocolServiceType t);
  inline ProtocolServiceType ServiceType() const {return type_;}
  inline bool IsServerSideservice() const {return type_ == ProtocolServiceType::kServer;};
  inline MessageType InComingMessageType() const {return IsServerSideservice()? MessageType::kRequest : MessageType::kResponse;};
protected:
  ProtocolServiceType type_;
  ChannelWriter* writer_;
  ProtoMessageHandler message_handler_;
};

}
#endif
