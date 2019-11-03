#ifndef _LT_NET_CLIENT_CHANNEL_H
#define _LT_NET_CLIENT_CHANNEL_H

#include "url_utils.h"
#include "net_callback.h"
#include "client_base.h"
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
    virtual const url::RemoteInfo& GetRemoteInfo() const = 0;
    virtual const ClientConfig& GetClientConfig() const = 0;
    virtual uint32_t HeartBeatInterval() const {return 0;};

    virtual void OnClientChannelInited(const ClientChannel* channel) = 0;
    virtual void OnClientChannelClosed(const RefClientChannel& channel) = 0;
    virtual void OnRequestGetResponse(const RefProtocolMessage&, const RefProtocolMessage&) = 0;
  };

  enum State {
    kInitialing   = 0,
    kReady        = 1,
    kClosing      = 2,
    kDisconnected = 3
  };

  ClientChannel(Delegate* d, const RefProtoService& service);
  virtual ~ClientChannel();

  virtual void StartClient();
  virtual void SendRequest(RefProtocolMessage request) = 0;

  void Close();
  void ResetDelegate();

  bool Ready() const {return state_ == kReady;}
  bool Closing() const {return state_ == kClosing;}
  bool Initializing() const {return state_ == kInitialing;}

  void SetRequestTimeout(uint32_t ms) {request_timeout_ = ms;};
  base::MessageLoop* IOLoop() {return protocol_service_->IOLoop();};

  // a change for close all inprogress request
  virtual void BeforeCloseChannel() = 0;
  //override from ProtoServiceDelegate
  virtual void OnProtocolServiceReady(const RefProtoService& service);
protected:
  Delegate* delegate_;
  State state_ = kInitialing;
  RefProtoService protocol_service_;
  uint32_t request_timeout_ = 5000; //5s
};

RefClientChannel CreateClientChannel(ClientChannel::Delegate*, const RefProtoService&);


}}//end namespace
#endif
