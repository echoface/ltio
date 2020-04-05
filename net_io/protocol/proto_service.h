#ifndef _NET_PROTOCOL_SERVICE_H_H
#define _NET_PROTOCOL_SERVICE_H_H

#include "base/message_loop/message_loop.h"
#include "proto_message.h"

#include <net_io/url_utils.h>
#include <net_io/net_callback.h>
#include <net_io/tcp_channel.h>

namespace lt {
namespace net {

class ProtoServiceDelegate {
public:
  virtual void OnProtocolServiceReady(const RefProtoService& service) {};
  virtual void OnProtocolServiceGone(const RefProtoService& service) = 0;
  virtual void OnProtocolMessage(const RefProtocolMessage& message) = 0;
  //for client side
  virtual const url::RemoteInfo* GetRemoteInfo() const {return NULL;};
};

/* a stateless encoder/decoder and
 * transfer the ProtoMessage to real Handler */
class ProtoService : public SocketChannel::Reciever,
                     public EnableShared(ProtoService) {
public:
  ProtoService(base::MessageLoop* loop);
  virtual ~ProtoService() {};

  void SetDelegate(ProtoServiceDelegate* d);

  /* this can be override
   * if need difference channel type,
   * eg SSLChannel, UdpChannel */
  virtual bool BindToSocket(int fd, const SocketAddr& local, const SocketAddr& peer);

  virtual void Initialize();

  TcpChannel* Channel() {return channel_.get();};
  base::MessageLoop* IOLoop() const { return binded_loop_;}

  void CloseService();
  bool IsConnected() const;

  virtual void BeforeCloseService() {};
  virtual void AfterChannelClosed() {};

  /* feature indentify*/
  //async clients request
  virtual bool KeepSequence() {return true;};
  virtual bool KeepHeartBeat() {return false;}

  virtual bool SendRequestMessage(ProtocolMessage* message) = 0;
  virtual bool SendResponseMessage(const ProtocolMessage* req, ProtocolMessage* res) = 0;

  virtual const RefProtocolMessage NewHeartbeat() {return NULL;}
  virtual const RefProtocolMessage NewResponse(const ProtocolMessage*) {return NULL;}

  virtual const std::string& protocol() const {
    const static std::string kEmpty;
    return kEmpty;
  };

  void SetIsServerSide(bool server_side);
  bool IsServerSide() const {return server_side_;}
  inline MessageType InComingType() const {
    return server_side_ ? MessageType::kRequest : MessageType::kResponse;
  }
protected:
  // override this do initializing for client side, like set db, auth etc
  virtual void OnChannelReady(const SocketChannel*) override;

  void OnChannelClosed(const SocketChannel*) override;

  bool server_side_;
  RefTcpChannel channel_;
  ProtoServiceDelegate* delegate_ = nullptr;
  base::MessageLoop* binded_loop_ = nullptr;
};

}}
#endif
