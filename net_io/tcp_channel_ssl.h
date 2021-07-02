#ifndef _LT_COMPONENT_TCP_CHANNEL_SSL_H_
#define _LT_COMPONENT_TCP_CHANNEL_SSL_H_

#include <functional>
#include <memory>
#include "base/message_loop/event_pump.h"
#include "channel.h"
#include "codec/codec_message.h"
#include "net_callback.h"
#include "base/crypto/lt_ssl.h"

namespace lt {
namespace net {

class TCPSSLChannel;
using TCPSSLChannelPtr = std::unique_ptr<TCPSSLChannel>;

class TCPSSLChannel : public SocketChannel {
public:
  static TCPSSLChannelPtr Create(int socket_fd,
                                 const IPEndPoint& local,
                                 const IPEndPoint& peer);

  ~TCPSSLChannel();

  void InitSSL(SSLImpl* ssl);

  void StartChannel() override;

  void ShutdownChannel(bool half_close) override;

  void ShutdownWithoutNotify() override;

  int32_t Send(const char* data, const int32_t len) override;

protected:
  TCPSSLChannel(int socket_fd,
                const IPEndPoint& loc,
                const IPEndPoint& peer);

  bool OnHandshake(base::FdEvent* event);

  bool HandleWrite(base::FdEvent* event) override;

  bool HandleRead(base::FdEvent* event) override;
private:
  bool server_ = true;
  bool handshake_done_ = false;

#ifdef LTIO_WITH_OPENSSL
  SSLImpl* ssl_;
#endif

  DISALLOW_COPY_AND_ASSIGN(TCPSSLChannel);
};

}  // namespace net
}  // namespace lt

#endif
