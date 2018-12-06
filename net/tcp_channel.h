#ifndef NET_TCP_CHANNAL_CONNECTION_H_
#define NET_TCP_CHANNAL_CONNECTION_H_

#include <memory>
#include <functional>

#include "net_callback.h"
#include "io_buffer.h"
#include "inet_address.h"
#include "base/base_micro.h"
#include "protocol/proto_message.h"
#include "base/message_loop/message_loop.h"
#include "channel.h"
/* *
 *
 * all of this thing happend in io-loop its attached, include callbacks
 *
 * */
namespace net {

class TcpChannel : public std::enable_shared_from_this<TcpChannel> {
public:
  typedef enum {
    CONNECTING = 0,
    CONNECTED = 1,
    DISCONNECTED = 2,
    DISCONNECTING = 3
  } ChannelStatus;

  static RefTcpChannel Create(int socket_fd,
                              const InetAddress& local,
                              const InetAddress& peer,
                              base::MessageLoop* loop);

  TcpChannel(int socket_fd,
             const InetAddress& loc,
             const InetAddress& peer,
             base::MessageLoop* loop);

  ~TcpChannel();

  void Start();

  void SetChannelName(const std::string name);
  const std::string& ChannelName() const {return channal_name_;}

  void SetChannelConsumer(ChannelConsumer* consumer);
  void SetCloseCallback(const ChannelClosedCallback& callback);

  /* return -1 in error, return 0 when success*/
  int32_t Send(const uint8_t* data, const int32_t len);

  void ForceShutdown();
  void ShutdownChannel();
  bool IsConnected() const;

  bool InIOLoop() const;
  base::MessageLoop* IOLoop() const;
  const std::string StatusAsString() const;
protected:
  void HandleRead();
  void HandleWrite();
  void HandleError();
  void HandleClose();

private:
  void OnConnectionReady();
  void SetChannelStatus(ChannelStatus st);

  /* channel relative things should happened on io_loop*/
  base::MessageLoop* io_loop_;

  bool schedule_shutdown_;
  ChannelStatus channel_status_ = CONNECTING;

  base::RefFdEvent fd_event_;

  InetAddress local_addr_;
  InetAddress peer_addr_;

  std::string channal_name_;

  IOBuffer in_buffer_;
  IOBuffer out_buffer_;

  ChannelConsumer* channel_consumer_ = NULL;

  ChannelClosedCallback closed_callback_;
  DISALLOW_COPY_AND_ASSIGN(TcpChannel);
};

}
#endif
