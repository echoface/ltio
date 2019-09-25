#ifndef LT_NET_CLIENT_H_H
#define LT_NET_CLIENT_H_H

#include <vector>

#include <memory>
#include <chrono>             // std::chrono::seconds
#include <mutex>              // std::mutex, std::unique_lock
#include <cinttypes>
#include <condition_variable> // std::condition_variable, std::cv_status
#include <url_utils.h>

#include "address.h"
#include "tcp_channel.h"
#include "socket_utils.h"
#include "socket_acceptor.h"
#include "protocol/proto_service.h"
#include "protocol/proto_message.h"
#include "protocol/proto_service_factory.h"

#include "client_base.h"
#include "clients/queued_channel.h"
#include "clients/client_connector.h"
#include "initializer/initializer.h"

namespace lt {
namespace net {

class ClientDelegate {
public:
  virtual base::MessageLoop* NextIOLoopForClient() = 0;
  /* do some init things like select db for redis,
   * or enable keepalive action etc*/
  virtual void OnClientChannelReady(ClientChannel* channel) {}
  virtual void OnClientChannelGone(ClientChannel* channel) {}
};

class Client;
typedef std::shared_ptr<Client> RefClient;
typedef std::function<void(ProtocolMessage*)> AsyncCallBack;

class Client: public ConnectorDelegate,
              public Initializer::Provider,
              public ClientChannel::Delegate {
public:
  Client(base::MessageLoop*, const url::RemoteInfo&);
  virtual ~Client();

  void SetDelegate(ClientDelegate* delegate);
  void Initialize(const ClientConfig& config);

  void Finalize();
  void FinalizeSync();

  template<class T>
  typename T::element_type::ResponseType* SendRecieve(T& m) {
    RefProtocolMessage message = std::static_pointer_cast<ProtocolMessage>(m);
    return (typename T::element_type::ResponseType*)(SendClientRequest(message));
  }
  ProtocolMessage* SendClientRequest(RefProtocolMessage& message);

  bool AsyncSendRequest(RefProtocolMessage& req, AsyncCallBack);

  //override from ConnectorDelegate
  void OnClientConnectFailed() override;
  void OnNewClientConnected(int fd, SocketAddr& loc, SocketAddr& remote) override;

  //override Initializer::Provider
  void OnClientServiceReady(const RefProtoService&) override;
  const ClientConfig& GetClientConfig() const override {return config_;}
  const url::RemoteInfo& GetRemoteInfo() const override {return remote_info_;}

  //override from ClientChannel::Delegate
  uint32_t HeartBeatInterval() const override {return config_.heart_beat_ms;}
  void OnClientChannelClosed(const RefClientChannel& channel) override;
  void OnRequestGetResponse(const RefProtocolMessage&, const RefProtocolMessage&) override;

  uint64_t ClientCount() const;
  std::string ClientInfo() const;
  std::string RemoteIpPort() const;
private:
  //Get a io work loop for channel, if no loop provide, use default io_loop_;
  base::MessageLoop* GetLoopForClient();

  SocketAddr address_;
  const url::RemoteInfo remote_info_;

  /* workloop mean: all client channel born & die, not mean
   * clientchannel io loop, client channel may work in other loop*/
  base::MessageLoop* work_loop_;

  bool is_stopping_;

  //sync start or stop
  std::mutex mtx_;
  std::condition_variable cv_;

  ClientConfig config_;
  RefConnector connector_;
  ClientDelegate* delegate_;
  Initializer* initializer_ = NULL;
  typedef std::vector<RefClientChannel> ClientChannelList;
  typedef std::shared_ptr<ClientChannelList> RefClientChannelList;

  /*channels_ only access & modify in this client worker loop*/
  ClientChannelList channels_;

  //a channels copy for client caller
  std::atomic<uint32_t> next_index_;
  RefClientChannelList roundrobin_channes_;
};

}}//end namespace lt::net
#endif
