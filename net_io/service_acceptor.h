#ifndef _NET_SERVICE_ACCEPTOR_H_H_
#define _NET_SERVICE_ACCEPTOR_H_H_

#include "inet_address.h"
#include "socket_utils.h"

#include "net_callback.h"
#include "base/base_micro.h"
#include "base/message_loop/fd_event.h"
#include "base/message_loop/message_loop.h"

namespace net {

class SocketAcceptor {
public:
  SocketAcceptor(base::EventPump*, const SocketAddress&);
  ~SocketAcceptor();

  bool StartListen();
  void StopListen();
  bool IsListening() { return listening_; }

  void SetNewConnectionCallback(const NewConnectionCallback& cb);
  const SocketAddress& ListeningAddress() const { return address_; };
private:
  bool InitListener();
  void OnAcceptorError();
  void HandleCommingConnection();

  bool listening_;
  SocketAddress address_;

  base::EventPump* event_pump_;
  base::RefFdEvent socket_event_;
  NewConnectionCallback new_conn_callback_;
  DISALLOW_COPY_AND_ASSIGN(SocketAcceptor);
};

} //end net
#endif
//end of file
