/*
 * Copyright 2021 <name of copyright holder>
 * Author: Huan.Gong <gonghuan.dev@gmail.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef LT_NET_CLIENT_H_H
#define LT_NET_CLIENT_H_H

#include <atomic>
#include <chrono>  // std::chrono::seconds
#include <cinttypes>
#include <list>
#include <memory>
#include <mutex>  // std::mutex, std::unique_lock
#include <vector>

#include "client_base.h"
#include "client_channel.h"
#include "client_connector.h"
#include "interceptor.h"
#include "queued_channel.h"

#include <base/ltio_config.h>
#include <net_io/url_utils.h>
#include "net_io/codec/codec_factory.h"
#include "net_io/codec/codec_message.h"
#include "net_io/codec/codec_service.h"
#include "net_io/socket_acceptor.h"
#include "net_io/socket_utils.h"

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
  virtual void OnAllClientPassiveBroken(const Client* client){};
};

class Client : public Connector::Delegate,
#ifdef LTIO_HAVE_SSL
               public ClientSSLCtxProvider,
#endif
               public ClientChannel::Delegate {
public:
  typedef std::vector<RefClientChannel> ClientChannelList;
  typedef std::shared_ptr<ClientChannelList> RefClientChannelList;

  Client(base::MessageLoop*, const url::RemoteInfo&);

  virtual ~Client();

  void Finalize();

  void Initialize(const ClientConfig& config);

  void SetDelegate(ClientDelegate* delegate);

  Client* Use(Interceptor* interceptor);

  Client* Use(InterceptArgList interceptors);

  template <class T>
  typename T::element_type::ResponseType* SendRecieve(T& m) {
    RefCodecMessage message = std::static_pointer_cast<CodecMessage>(m);
    return (typename T::element_type::ResponseType*)(DoRequest(message));
  }

  CodecMessage* DoRequest(RefCodecMessage& message);

  bool AsyncDoRequest(const RefCodecMessage& req, AsyncCallBack);

  // notified from connector
  void OnConnectFailed(uint32_t count) override;
  // notified from connector
  void OnConnected(int fd, IPEndPoint& loc, IPEndPoint& remote) override;

  // clientchanneldelegate
  const ClientConfig& GetClientConfig() const override { return config_; }

  const url::RemoteInfo& GetRemoteInfo() const override { return remote_info_; }

  void OnClientChannelInited(const ClientChannel* channel) override;

  // only called when passive close by peer,
  // active close won't be notified for thread-safe reason
  void OnClientChannelClosed(const RefClientChannel& channel) override;

  void OnRequestGetResponse(const RefCodecMessage&,
                            const RefCodecMessage&) override;

  uint64_t ConnectedCount() const;

  std::string ClientInfo() const;

  std::string RemoteIpPort() const;

private:
  void launch_next_if_need();

  uint32_t required_count() const;

  /*return a connected and initialized channel*/
  RefClientChannel get_ready_channel();

  /*return a io loop for client channel work on*/
  base::MessageLoop* next_client_io_loop();

  IPEndPoint address_;

  const url::RemoteInfo remote_info_;

  /* NOTE:
   * A loop manager ClientChannel's lifecycle
   * not mean the client message io loop
   * */
  base::MessageLoop* work_loop_;

  std::atomic<bool> stopping_;

  ClientConfig config_;
  RefConnector connector_;
  ClientDelegate* delegate_;
  // a channels copy for client caller
  std::atomic<uint32_t> next_index_;
  RefClientChannelList in_use_channels_;

  // full list for manager purpose, once changed
  // a copy of update set to in_use_channels_;
  std::atomic<uint32_t> channels_count_;
  std::list<RefClientChannel> channels_;

  /* reset to zero when a success connected
   * otherwise increase utill kMaxReconInterval*/
  uint32_t next_reconnect_interval_;
  static const uint32_t kMaxReconInterval;  // in ms
  DISALLOW_COPY_AND_ASSIGN(Client);
};

}  // namespace net
}  // namespace lt
#endif
