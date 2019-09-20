#ifndef NET_TCP_CHANNAL_CONNECTION_H_
#define NET_TCP_CHANNAL_CONNECTION_H_

#include <memory>
#include <functional>

#include "net_callback.h"
#include "io_buffer.h"
#include "address.h"
#include "base/base_micro.h"
#include "protocol/proto_message.h"
#include "base/message_loop/message_loop.h"
#include "channel.h"
/* *
 * all of this thing happend in io-loop its attached, include callbacks
 * */
namespace lt {
namespace net {

class TcpChannel : public SocketChannel,
                   public std::enable_shared_from_this<TcpChannel> {
public:
  enum class Status {
    CONNECTING,
    CONNECTED,
	  CLOSING,
    CLOSED
  };

  static RefTcpChannel Create(int socket_fd,
                              const SocketAddress& local,
                              const SocketAddress& peer,
                              base::MessageLoop* loop);

  TcpChannel(int socket_fd,
             const SocketAddress& loc,
             const SocketAddress& peer,
             base::MessageLoop* loop);

  ~TcpChannel();

  void Start();

  const std::string& ChannelName() const {return name_;}
  void SetReciever(SocketChannel::Reciever* consumer);
  /* return -1 when error,
   * return 0 when all data pending to buffer,
   * other case return the byte writen to socket fd successfully */
  int32_t Send(const uint8_t* data, const int32_t len);

  void ShutdownChannel();
  bool IsConnected() const;
	std::string ChannelInfo() const;

  bool InIOLoop() const;
  base::MessageLoop* IOLoop() const;
  const std::string StatusAsString() const;
protected:
  void HandleRead();
  void HandleWrite();
  void HandleError();
  void HandleClose();

private:
  void OnChannelReady();
  void SetChannelStatus(Status st);

  /* channel relative things should happened on io_loop*/
  base::MessageLoop* io_loop_;

  bool schedule_shutdown_;
  Status status_ = Status::CLOSED;

  base::RefFdEvent fd_event_;

  SocketAddress local_addr_;
  SocketAddress peer_addr_;

  std::string name_;

  IOBuffer in_buffer_;
  IOBuffer out_buffer_;

  SocketChannel::Reciever* reciever_ = NULL;
  DISALLOW_COPY_AND_ASSIGN(TcpChannel);
};

}}
#endif
