#ifndef _NET_PROTOCOL_SERVICE_H_H
#define _NET_PROTOCOL_SERVICE_H_H

#include "../net_callback.h"
#include "proto_message.h"
#include "net/tcp_channel.h"
#include "net/channel.h"

namespace net {

enum class ProtocolServiceType{
  kServer,
  kClient
};

class ProtoServiceDelegate {
public:
  virtual void OnProtocolServiceGone(const RefProtoService& service) = 0;
};

/* a stateless encoder/decoder and
 * transfer the ProtoMessage to real Handler */
class ProtoService : public ChannelConsumer,
                     public std::enable_shared_from_this<ProtoService> {
public:
  ProtoService();
  virtual ~ProtoService();

  void SetDelegate(ProtoServiceDelegate* d);
  void BindChannel(RefTcpChannel& channel);
  void SetMessageHandler(ProtoMessageHandler);
  TcpChannel* Channel() {return channel_.get();};
  base::MessageLoop* IOLoop() {return channel_ ? channel_->IOLoop() : NULL;}

  void CloseService();
  bool IsConnected() const;
  virtual void BeforeCloseService() {};

  //async clients request
  virtual bool KeepSequence() {return true;};

  virtual bool SendRequestMessage(const RefProtocolMessage& message) = 0;
  virtual bool ReplyRequest(const RefProtocolMessage& req, const RefProtocolMessage& res) = 0;

  virtual bool CloseAfterMessage(ProtocolMessage*, ProtocolMessage*) { return true;};
  virtual const RefProtocolMessage NewResponseFromRequest(const RefProtocolMessage &) {return NULL;}

  void SetServiceType(ProtocolServiceType t);
  inline ProtocolServiceType ServiceType() const {return type_;}
  inline bool IsServerService() const {return type_ == ProtocolServiceType::kServer;};
  inline MessageType InComingType() const {return IsServerService()? MessageType::kRequest : MessageType::kResponse;};

  virtual void AfterChannelClosed() {};
protected:
  void OnChannelClosed(const RefTcpChannel&) override;
protected:
  ProtocolServiceType type_;
  RefTcpChannel channel_;
  ProtoServiceDelegate* delegate_ = NULL;
  ProtoMessageHandler message_handler_;
};

}
#endif
