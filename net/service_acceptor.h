#ifndef _NET_SERVICE_ACCEPTOR_H_H_
#define _NET_SERVICE_ACCEPTOR_H_H_

#include "inet_address.h"
#include "socket_utils.h"

#include "net_callback.h"
#include "base/base_micro.h"
#include "base/message_loop/fd_event.h"
#include "base/message_loop/message_loop.h"

namespace net {

class ServiceAcceptor {
public:
  ServiceAcceptor(base::EventPump*, const InetAddress&);
  ~ServiceAcceptor();

  bool StartListen();
  void StopListen();
  bool IsListenning() { return listenning_; }

  void SetNewConnectionCallback(const NewConnectionCallback& cb);
  const InetAddress& ListenningAddress() const { return address_; };
private:
  bool InitListener();
  void OnAcceptorError();
  void HandleCommingConnection();

  bool listenning_;
  InetAddress address_;

  base::EventPump* event_pump_;
  base::FdEvent::RefFdEvent socket_event_;
  NewConnectionCallback new_conn_callback_;
  DISALLOW_COPY_AND_ASSIGN(ServiceAcceptor);
};

} //end net
#endif
//end of file
