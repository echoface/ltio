#include "ws_client.h"

#include <base/utils/string/str_utils.h>
#include <net_io/tcp_channel.h>

#ifdef LTIO_HAVE_SSL
#include <net_io/tcp_channel_ssl.h>
#endif

using base::StrUtil;

namespace lt {
namespace net {

WsClient::RefClient WsClient::New(MessageLoop* io) {
  return RefClient(new WsClient(io));
}

WsClient::WsClient(MessageLoop* io)
  : io_loop_(io),
    dialer_(new Connector(io->Pump())) {
}

WsClient::~WsClient() {
  io_loop_->PostTask(FROM_HERE, &Connector::Stop, dialer_);
  io_loop_->PostTask(FROM_HERE, &WSCodecService::CloseService, transport_, true);
}

WsClient::Error WsClient::Dial(const std::string& addr, const std::string& topic) {
  CHECK(StrUtil::StartsWith(addr, "ws") || StrUtil::StartsWith(addr, "wss"));
  topic_path_ = topic;
  bool ok = url::ParseRemote(addr, remote_, true);
  if (!ok || remote_.host_ip.empty()) {
    return kBadAddress;
  }
  IPEndPoint ep(remote_.host_ip, remote_.port);
  io_loop_->PostTask(FROM_HERE, &Connector::Dial, dialer_, ep, this);
  return kSuccess;
}

WsClient::Error WsClient::DialSync(const std::string& addr) {

  CHECK(StrUtil::StartsWith(addr, "ws") || StrUtil::StartsWith(addr, "wss"));

  bool ok = url::ParseRemote(addr, remote_, true);
  if (!ok || remote_.host_ip.empty())
    return kBadAddress;

  IPEndPoint ep(remote_.host_ip, remote_.port);
  ConnDetail conn_detail = dialer_->DialSync(ep);
  if (!conn_detail.valid()) {
    return kSrvRefused;
  }
  OnConnected(conn_detail.socket, conn_detail.local, conn_detail.peer);
  return kSuccess;
}

void WsClient::Send(const RefWebsocketFrame& msg) {
  if (!io_loop_->IsInLoopThread()) {
    RefClient guard = shared_from_this();
    io_loop_->PostTask(FROM_HERE, &WsClient::Send, guard, msg);
    return;
  }

  RefClient guard = shared_from_this();
  if (!transport_ || !transport_->IsConnected()) {
    if (error_callback_) {
      VLOG(VTRACE) << "transport_ broken";
      error_callback_(guard, kConnBroken);
    }
    return;
  }

  if (!transport_->Send(msg)) {
    transport_->CloseService(true);
    if (error_callback_) {
      error_callback_(guard, kConnBroken);
    }
  }
}

void WsClient::OnConnectFailed(uint32_t count) {
  if (error_callback_) {
    error_callback_(shared_from_this(), kSrvRefused);
  }
}

void WsClient::OnConnected(int socket_fd,
                                  IPEndPoint& local,
                                  IPEndPoint& remote) {
  CHECK(io_loop_->IsInLoopThread());

  transport_ = std::make_shared<WSCodecService>(io_loop_);
  transport_->SetHandler(this);
  transport_->SetDelegate(this);
  transport_->SetIsServerSide(false);
  transport_->SetProtocol(remote_.scheme);
  transport_->SetTopicPath(topic_path_);

  SocketChannelPtr channel;
  if (transport_->UseSSLChannel()) {
#ifdef LTIO_HAVE_SSL
    auto ch = TCPSSLChannel::Create(socket_fd, local, remote);
    ch->InitSSL(GetClientSSLContext()->NewSSLSession(socket_fd));
    channel = std::move(ch);
#else
    CHECK(false) << "ssl need compile with openssl lib support";
#endif
  } else {
    channel = TcpChannel::Create(socket_fd, local, remote);
  }

  auto fdev = base::FdEvent::Create(nullptr, socket_fd, base::LtEv::READ);

  transport_->BindSocket(std::move(fdev), std::move(channel));

  transport_->StartProtocolService();
}

void WsClient::OnCodecMessage(const RefCodecMessage& message) {
  if (msg_handler_) {
    msg_handler_(shared_from_this(), RefCast(WebsocketFrame, message));
  }
}

void WsClient::OnCodecReady(const RefCodecService& service) {
  if (open_callback_) {
    open_callback_(shared_from_this());
  }
}

void WsClient::OnCodecClosed(const RefCodecService& service) {
  if (close_callback_) {
    close_callback_(shared_from_this());
  }
}

const url::RemoteInfo* WsClient::GetRemoteInfo() const {
  return &remote_;
}

}  // namespace net
};  // namespace lt
