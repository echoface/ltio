#ifndef NET_TCP_CHANNAL_CONNECTION_H_
#define NET_TCP_CHANNAL_CONNECTION_H_

#include <memory>
#include <functional>

#include "channel.h"
#include "net_callback.h"
#include "protocol/proto_message.h"
/* *
 * all of this thing happend in io-loop its attached, include callbacks
 * */
namespace lt {
namespace net {

class TcpChannel : public SocketChannel,
                   public std::enable_shared_from_this<TcpChannel> {
public:
  static RefTcpChannel Create(int socket_fd,
                              const SocketAddr& local,
                              const SocketAddr& peer,
                              base::MessageLoop* loop);
  ~TcpChannel();

  int32_t Send(const uint8_t* data, const int32_t len) override;
  void ShutdownChannel(bool half_close) override;
protected:
  TcpChannel(int socket_fd,
             const SocketAddr& loc,
             const SocketAddr& peer,
             base::MessageLoop* loop);

  void HandleRead();
  void HandleWrite();
  void HandleError();
  void HandleClose();

private:
  DISALLOW_COPY_AND_ASSIGN(TcpChannel);
};

}}
#endif
