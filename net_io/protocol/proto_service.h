#ifndef _NET_PROTOCOL_SERVICE_H_H
#define _NET_PROTOCOL_SERVICE_H_H

#include "channel.h"
#include "net_callback.h"
#include "tcp_channel.h"
#include "url_utils.h"
#include "proto_message.h"

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
  ProtoService();
  virtual ~ProtoService();

  void SetDelegate(ProtoServiceDelegate* d);
  /* this can be override
   * if need difference channel type,
   * eg SSLChannel, UdpChannel */
  virtual bool BindToSocket(int fd,
                            const SocketAddr& local,
                            const SocketAddr& peer,
                            base::MessageLoop* loop);

  virtual void Initialize();

  TcpChannel* Channel() {return channel_.get();};
  base::MessageLoop* IOLoop() {return channel_ ? channel_->IOLoop() : NULL;}

  void CloseService();
  bool IsConnected() const;

  virtual void BeforeCloseService() {};
  virtual void AfterChannelClosed() {};

  /* feature indentify*/
  //async clients request
  virtual bool KeepSequence() {return true;};
  virtual bool KeepHeartBeat() {return false;}

  virtual bool SendRequestMessage(const RefProtocolMessage& message) = 0;
  virtual bool SendResponseMessage(const RefProtocolMessage& req, const RefProtocolMessage& res) = 0;

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
  ProtoServiceDelegate* delegate_ = NULL;
};

}}
#endif
