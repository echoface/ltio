#ifndef _LT_NET_CLIENT_WEBSOCKET_H_
#define _LT_NET_CLIENT_WEBSOCKET_H_

#include "client_base.h"
#include "client_connector.h"

#include <base/message_loop/message_loop.h>
#include <net_io/codec/websocket/ws_codec_service.h>

namespace lt {
namespace net {

/*
 * WebsocketClient client(io);
 * client.SetOpenCallback(fun);
 * client.SetCloseCallback(fun);
 * client.SetMsgCallback(onmessage);
 *
 * lt::net::Connector dialer(io, client);
 * dialer.Dial("127.0.0.1:5006", &client);
 *
 * */

using base::MessageLoop;

class WsClient : public Connector::Delegate,
                 public CodecService::Handler,
                 public CodecService::Delegate,
#ifdef LTIO_HAVE_SSL
                 public ClientSSLCtxProvider,
#endif
                 public EnableShared(WsClient) {
public:
  enum Error {
    kSuccess = 0,
    kBadAddress = 1,
    kSrvRefused = 2,
    kConnBroken = 3,
  };

  using RefClient = std::shared_ptr<WsClient>;
  using Callback = std::function<void(const RefClient&)>;
  using ErrCallback = std::function<void(const RefClient&, Error err)>;
  using MsgCallback =
      std::function<void(const RefClient&, const RefWebsocketFrame&)>;

  static RefClient New(MessageLoop* io);

  ~WsClient();

  WsClient& WithRetries(int n) {
    max_retries_ = std::max(n, 3);
    return *this;
  }

  WsClient& WithOpen(const Callback& cb) {
    open_callback_ = cb;
    return *this;
  }

  WsClient& WithClose(const Callback& cb) {
    close_callback_ = cb;
    return *this;
  }

  WsClient& WithError(const ErrCallback& cb) {
    error_callback_ = cb;
    return *this;
  }

  WsClient& WithMessage(const MsgCallback& cb) {
    msg_handler_ = cb;
    return *this;
  }

  Error Dial(const std::string& addr, const std::string& topic);

  Error DialSync(const std::string& addr);

  void Send(const RefWebsocketFrame& msg);

private:
  using RefWsCodecService = std::shared_ptr<WSCodecService>;

  WsClient(base::MessageLoop* io);

  void OnCodecMessage(const RefCodecMessage& message) override;

  void OnCodecReady(const RefCodecService& service) override;

  void OnCodecClosed(const RefCodecService& service) override;

  const url::RemoteInfo* GetRemoteInfo() const override;

  void OnConnectFailed(uint32_t count) override;

  void OnConnected(int socket_fd,
                   IPEndPoint& local,
                   IPEndPoint& remote) override;

  MessageLoop* io_loop_;

  // ref ensure safe close
  RefConnector dialer_;

  RefWsCodecService transport_;

  url::RemoteInfo remote_;

  // upgrade message request path
  std::string topic_path_;

  int max_retries_ = 0;

  Callback open_callback_;
  Callback close_callback_;
  ErrCallback error_callback_;
  MsgCallback msg_handler_;
  DISALLOW_COPY_AND_ASSIGN(WsClient);
};

}  // namespace net
}  // namespace lt

#endif
