#ifndef NET_TCP_CHANNAL_CONNECTION_H_
#define NET_TCP_CHANNAL_CONNECTION_H_

#include <memory>
#include <functional>

#include "base/message_loop/event_pump.h"
#include "channel.h"
#include "net_callback.h"
#include "codec/codec_message.h"
/* *
 * all of this thing happend in io-loop its attached, include callbacks
 * */
namespace lt {
namespace net {

class TcpChannel : public SocketChannel,
                   public EnableShared(TcpChannel) {
public:
  static RefTcpChannel Create(int socket_fd,
                              const SocketAddr& local,
                              const SocketAddr& peer,
                              base::EventPump* pump);
  ~TcpChannel();

  void ShutdownChannel(bool half_close) override;
  int32_t Send(const char* data, const int32_t len) override;
protected:
  TcpChannel(int socket_fd,
             const SocketAddr& loc,
             const SocketAddr& peer,
             base::EventPump* pump);

  void HandleRead();
  void HandleWrite();
  void HandleError();
  void HandleClose();

private:
  DISALLOW_COPY_AND_ASSIGN(TcpChannel);
};

}}
#endif
