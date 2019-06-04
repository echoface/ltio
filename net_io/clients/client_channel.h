#ifndef _LT_NET_CLIENT_CHANNEL_H
#define _LT_NET_CLIENT_CHANNEL_H

#include "tcp_channel.h"
#include "net_callback.h"
#include "protocol/proto_message.h"
#include "protocol/proto_service.h"

namespace lt {
namespace net {

class ClientChannel;

typedef std::shared_ptr<ClientChannel> RefClientChannel;
class ClientChannel : public ProtoServiceDelegate {
public:
  class Delegate {
  public:
    virtual uint32_t HeartBeatInterval() const {return 0;};
    virtual void OnClientChannelClosed(const RefClientChannel& channel) = 0;
    virtual void OnRequestGetResponse(const RefProtocolMessage&, const RefProtocolMessage&) = 0;
  };

  ClientChannel(Delegate* d, RefProtoService& service);

  virtual ~ClientChannel() {}

  virtual void StartClient();
  virtual void SendRequest(RefProtocolMessage request) = 0;

  void CloseClientChannel();
  void SetRequestTimeout(uint32_t ms) {request_timeout_ = ms;};
  base::MessageLoop* IOLoop() {return protocol_service_->IOLoop();};
protected:
  Delegate* delegate_;
  RefProtoService protocol_service_;
  uint32_t request_timeout_ = 5000; //1s
};

RefClientChannel CreateClientChannel(ClientChannel::Delegate*, RefProtoService&);


}}//end namespace
#endif
