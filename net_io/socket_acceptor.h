#ifndef _NET_SERVICE_ACCEPTOR_H_H_
#define _NET_SERVICE_ACCEPTOR_H_H_

#include "socket_utils.h"
#include "base/ip_endpoint.h"

#include "net_callback.h"
#include "base/base_micro.h"
#include "base/message_loop/fd_event.h"
#include "base/message_loop/message_loop.h"

namespace lt {
namespace net {

class SocketAcceptor : public base::FdEvent::Handler {
public:
  SocketAcceptor(base::EventPump*, const IPEndPoint&);
  ~SocketAcceptor();

  bool StartListen();
  void StopListen();
  bool IsListening() { return listening_; }

  void SetNewConnectionCallback(const NewConnectionCallback& cb);
  const IPEndPoint& ListeningAddress() const { return address_; };
private:
  bool InitListener();
  //override from FdEvent::Handler
  void HandleRead(base::FdEvent* fd_event) override;
  void HandleWrite(base::FdEvent* fd_event) override;
  void HandleError(base::FdEvent* fd_event) override;
  void HandleClose(base::FdEvent* fd_event) override;

  bool listening_;
  IPEndPoint address_;

  base::EventPump* event_pump_;
  base::RefFdEvent socket_event_;
  NewConnectionCallback new_conn_callback_;
  DISALLOW_COPY_AND_ASSIGN(SocketAcceptor);
};

}} //end net
#endif
//end of file
