#ifndef LT_NET_CLIENT_ROUTER_H_H
#define LT_NET_CLIENT_ROUTER_H_H

#include <vector>

#include <memory>
#include <chrono>             // std::chrono::seconds
#include <mutex>              // std::mutex, std::unique_lock
#include <cinttypes>
#include <condition_variable> // std::condition_variable, std::cv_status
#include <url_string_utils.h>

#include "inet_address.h"
#include "tcp_channel.h"
#include "socket_utils.h"
#include "service_acceptor.h"
#include "url_string_utils.h"
#include "protocol/proto_service.h"
#include "protocol/proto_message.h"
#include "protocol/proto_service_factory.h"
#include "clients/queued_channel.h"
#include "clients/client_router.h"
#include "clients/client_connector.h"
#include "dispatcher/coro_dispatcher.h"

namespace net {

typedef struct {
  uint32_t connections = 1;
  // heart beat_ms only work for protocol service supported
  // 0 mean not need heart beat keep
  uint32_t heart_beat_ms = 0;

  uint32_t recon_interval = 5000;

  uint32_t message_timeout = 5000;
} RouterConf;

class RouterDelegate {
public:
  virtual base::MessageLoop* NextIOLoopForClient() = 0;
};

class ClientRouter;
typedef std::shared_ptr<ClientRouter> RefClientRouter;

class ClientRouter : public ConnectorDelegate,
                     public ClientChannel::Delegate {
public:
  ClientRouter(base::MessageLoop*, const url::SchemeIpPort&);
  ~ClientRouter();
  void SetDelegate(RouterDelegate* delegate);
  void SetupRouter(const RouterConf& config);
  void SetWorkLoadTransfer(CoroWlDispatcher* dispatcher);

  void StartRouter();
  void StopRouter();
  void StopRouterSync();

  template<class T>
  typename T::element_type::ResponseType* SendRecieve(T& m) {
    RefProtocolMessage message = std::static_pointer_cast<ProtocolMessage>(m);
    return (typename T::element_type::ResponseType*)(SendClientRequest(message));
  }
  ProtocolMessage* SendClientRequest(RefProtocolMessage& message);

  //override from ConnectorDelegate
  void OnClientConnectFailed() override;
  void OnNewClientConnected(int fd, SocketAddress& loc, SocketAddress& remote) override;

  //override from ClientChannel::Delegate
  void OnClientChannelClosed(const RefClientChannel& channel) override;
  void OnRequestGetResponse(const RefProtocolMessage&, const RefProtocolMessage&) override;

  uint64_t ClientCount() const;
  std::string RouterInfo() const;
private:
  //Get a io work loop for channel, if no loop provide, use default io_loop_;
  base::MessageLoop* GetLoopForClient();

  SocketAddress address_;
  const url::SchemeIpPort server_info_;

  base::MessageLoop* work_loop_;

  bool is_stopping_;
  std::mutex mtx_;
  std::condition_variable cv_;

  RouterConf config_;
  RefConnector connector_;
  RouterDelegate* delegate_;
  CoroWlDispatcher* dispatcher_;
  typedef std::vector<RefClientChannel> ClientChannelList;

  ClientChannelList channels_;
  std::atomic<uint32_t> router_counter_;
  std::shared_ptr<ClientChannelList> roundrobin_channes_;
};

}//end namespace net
#endif
