//
// Created by gh on 18-12-5.
//

#ifndef LIGHTINGIO_NET_CHANNEL_H
#define LIGHTINGIO_NET_CHANNEL_H

#include "address.h"
#include "base/message_loop/event_pump.h"
#include "io_buffer.h"
#include "net_callback.h"
#include "base/base_micro.h"
#include "base/message_loop/message_loop.h"
#include <cstdint>

// socket chennel interface and base class
namespace lt {
namespace net {


class SocketChannel {
public:
  enum class Status {
    CONNECTING,
    CONNECTED,
	  CLOSING,
    CLOSED
  };
  typedef struct Reciever {
    virtual void OnChannelReady(const SocketChannel*) {};
    virtual void OnStatusChanged(const SocketChannel*) {};
    virtual void OnDataFinishSend(const SocketChannel*) {};
    virtual void OnChannelClosed(const SocketChannel*) = 0;
    virtual void OnDataReceived(const SocketChannel*, IOBuffer *) = 0;
  }Reciever;

public:
  void StartChannel();
  void SetReciever(SocketChannel::Reciever* consumer);

  /* return -1 when error,
   * return 0 when all data pending to buffer,
   * other case return the byte of writen*/
  virtual int32_t Send(const char* data, const int32_t len) = 0;

  /* a initiative call from application level to clase this channel
   * 1. shutdown writer if writing is enable
   * 2. directly unregiste fdevent from pump and close socket*/
  virtual void ShutdownChannel(bool half_close) = 0;

  bool IsConnected() const {return status_ == Status::CONNECTED;};

  std::string ChannelInfo() const;
  const std::string StatusAsString() const;
  const std::string& ChannelName() const {return name_;}
protected:
  SocketChannel(int socket_fd,
                const SocketAddr& loc,
                const SocketAddr& peer,
                base::EventPump* pump);
  virtual ~SocketChannel() {
    CHECK(status_ == Status::CLOSED);
  }

  void setup_channel();
  void close_channel();
  int32_t binded_fd() const;
  std::string local_name() const;
  std::string remote_name() const;

  void SetChannelStatus(Status st);

  /* channel-relatived pump for event notify */
  base::EventPump* pump_;

  bool schedule_shutdown_ = false;

  base::RefFdEvent fd_event_;

  SocketAddr local_addr_;
  SocketAddr peer_addr_;

  std::string name_;

  IOBuffer in_buffer_;
  IOBuffer out_buffer_;

  SocketChannel::Reciever* reciever_ = NULL;
private:
  Status status_ = Status::CONNECTING;

  DISALLOW_COPY_AND_ASSIGN(SocketChannel);
};

}} //lt::net
#endif //LIGHTINGIO_NET_CHANNEL_H
