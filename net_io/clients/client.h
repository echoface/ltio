#ifndef LT_NET_CLIENT_H_H
#define LT_NET_CLIENT_H_H

#include <vector>
#include <memory>
#include <chrono>             // std::chrono::seconds
#include <mutex>              // std::mutex, std::unique_lock
#include <cinttypes>
#include <condition_variable> // std::condition_variable, std::cv_status

#include "client_base.h"
#include "client_channel.h"
#include "interceptor.h"
#include "queued_channel.h"
#include "client_connector.h"

#include <net_io/url_utils.h>
#include "net_io/tcp_channel.h"
#include "net_io/socket_utils.h"
#include "net_io/socket_acceptor.h"
#include "net_io/codec/codec_service.h"
#include "net_io/codec/codec_message.h"
#include "net_io/codec/codec_factory.h"

namespace lt {
namespace net {

/**
 设计上 Client 存在两个交互界面, 在扩展时设计上应该
 遵守这两个交互界面的设计, 避免出现不同loop建的耦合

 - 一个来自Connector
  - 处理来自connector的连接建立成功、失败、重试等逻辑

 - 一个来自 Woker的请求与Response
  - 处理请求/响应交互
  - 处理来自连接初始化连接成功失败、状态变化等逻辑
*/

class Client;
typedef std::shared_ptr<Client> RefClient;
typedef std::function<void(CodecMessage*)> AsyncCallBack;

class ClientDelegate {
public:
  virtual base::MessageLoop* NextIOLoopForClient() = 0;
  /* do some init things like select db for redis,
   * or enable keepalive action etc*/
  virtual void OnClientChannelReady(const ClientChannel* channel) {}
  /* when a abnormal broken happend, here a chance to notify
   * application level for handle this case, eg:remote server
   * shutdown,crash etc*/
  virtual void OnAllClientPassiveBroken(const Client* client) {};
};

class Client: public ConnectorDelegate,
              public ClientChannel::Delegate {
public:
public:
  Client(base::MessageLoop*, const url::RemoteInfo&);
  virtual ~Client();

  void Finalize();
  void Initialize(const ClientConfig& config);
  void SetDelegate(ClientDelegate* delegate);
  Client* Use(Interceptor* interceptor);
  Client* Use(InterceptArgList interceptors);

  template<class T>
  typename T::element_type::ResponseType* SendRecieve(T& m) {
    RefCodecMessage message = std::static_pointer_cast<CodecMessage>(m);
    return (typename T::element_type::ResponseType*)(DoRequest(message));
  }
  CodecMessage* DoRequest(RefCodecMessage& message);

  bool AsyncDoRequest(RefCodecMessage& req, AsyncCallBack);

  // notified from connector
  void OnClientConnectFailed() override;
  // notified from connector
  void OnNewClientConnected(int fd, IPEndPoint& loc, IPEndPoint& remote) override;

  // clientchanneldelegate
  const ClientConfig& GetClientConfig() const override {return config_;}
  const url::RemoteInfo& GetRemoteInfo() const override {return remote_info_;}
  void OnClientChannelInited(const ClientChannel* channel) override;
  //only called when passive close, active close won't be notified for thread-safe reason
  void OnClientChannelClosed(const RefClientChannel& channel) override;
  void OnRequestGetResponse(const RefCodecMessage&, const RefCodecMessage&) override;

  uint64_t ConnectedCount() const;
  std::string ClientInfo() const;
  std::string RemoteIpPort() const;
private:
  /*return a connected and initialized channel*/
  RefClientChannel get_ready_channel();

  /*return a io loop for client channel work on*/
  base::MessageLoop* next_client_io_loop();

  IPEndPoint address_;
  const url::RemoteInfo remote_info_;

  /* NOTE:
   * correct:
   *  client channel born & die
   * wrong:
   *  clientchannel's io_loop
   * */
  base::MessageLoop* work_loop_;

  std::atomic<bool> stopping_;

  ClientConfig config_;
  RefConnector connector_;
  ClientDelegate* delegate_;
  typedef std::vector<RefClientChannel> ClientChannelList;

  //a channels copy for client caller
  std::atomic<uint32_t> next_index_;

  REF_TYPEDEFINE(ClientChannelList);
  RefClientChannelList in_use_channels_;

  DISALLOW_COPY_AND_ASSIGN(Client);
};

}}//end namespace lt::net
#endif
