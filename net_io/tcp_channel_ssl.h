#ifndef _LT_COMPONENT_TCP_CHANNEL_SSL_H_
#define _LT_COMPONENT_TCP_CHANNEL_SSL_H_

#include <functional>
#include <memory>
#include "base/message_loop/event_pump.h"
#include "channel.h"
#include "codec/codec_message.h"
#include "net_callback.h"
#include "openssl/err.h"
#include "openssl/ssl.h"

namespace lt {
namespace net {

SSL_CTX* ssl_ctx_init(const char* cert, const char* key);

/* */
class TCPSSLChannel;
using TCPSSLChannelPtr = std::unique_ptr<TCPSSLChannel>;

class TCPSSLChannel : public SocketChannel {
public:
  static TCPSSLChannelPtr Create(int socket_fd,
                                 const IPEndPoint& local,
                                 const IPEndPoint& peer,
                                 base::EventPump* pump);

  ~TCPSSLChannel();

  void StartChannel() override;

  void ShutdownChannel(bool half_close) override;

  void ShutdownWithoutNotify() override;

  int32_t Send(const char* data, const int32_t len) override;

protected:
  TCPSSLChannel(int socket_fd,
                const IPEndPoint& loc,
                const IPEndPoint& peer,
                base::EventPump* pump);

  bool OnHandshake(base::FdEvent* event);

  bool HandleWrite(base::FdEvent* event) override;
  bool HandleRead(base::FdEvent* event) override;

private:
  bool server_ = true;
  bool handshake_done_ = false;

  SSL* ssl_ = nullptr;
  SSL_CTX* ssl_ctx_ = nullptr;

  DISALLOW_COPY_AND_ASSIGN(TCPSSLChannel);
};

}  // namespace net
}  // namespace lt

#endif
