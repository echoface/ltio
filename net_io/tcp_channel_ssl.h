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

  bool StartChannel(bool server) override WARN_UNUSED_RESULT;

  int HandleRead() override WARN_UNUSED_RESULT;

  // write as much as data into socket
  int HandleWrite() override WARN_UNUSED_RESULT;

  // send raw data to socket
  int32_t Send(const char* data, const int32_t len) override WARN_UNUSED_RESULT;

protected:
  TCPSSLChannel(int socket_fd,
                const IPEndPoint& loc,
                const IPEndPoint& peer);

  // return true when success, false when failed
  bool DoHandshake(base::FdEvent* event);

private:
#ifdef LTIO_WITH_OPENSSL
  SSLImpl* ssl_ = nullptr;
#endif

  DISALLOW_COPY_AND_ASSIGN(TCPSSLChannel);
};

}  // namespace net
}  // namespace lt

#endif
