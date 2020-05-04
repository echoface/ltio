#ifndef _LT_NET_CLIENT_CONNECTOR_H_H
#define _LT_NET_CLIENT_CONNECTOR_H_H

#include <set>

#include "../tcp_channel.h"
#include "../socket_utils.h"
#include "../socket_acceptor.h"
#include "../codec/codec_service.h"
#include "../codec/line/line_message.h"
#include "../codec/http/http_request.h"
#include "../codec/http/http_response.h"
#include "../codec/codec_factory.h"

namespace lt {
namespace net {

class Connector;

typedef std::shared_ptr<Connector> RefConnector;
typedef std::unique_ptr<Connector> OwnedConnector;
typedef std::weak_ptr<base::FdEvent> WeakPtrFdEvent;

class ConnectorDelegate {
public:
  virtual ~ConnectorDelegate() {};
  virtual void OnClientConnectFailed() = 0;
  virtual void OnNewClientConnected(int socket_fd, SocketAddr& local, SocketAddr& remote) = 0;
};

class Connector : public base::FdEvent::Handler {
public:
  Connector(base::EventPump* pump, ConnectorDelegate* delegate);
  ~Connector() {};

  void Stop();

  //TODO: add a connect timeout
  bool Launch(const net::SocketAddr &address);

  void OnWrite(WeakPtrFdEvent weak_fdevent);
  void OnError(WeakPtrFdEvent weak_fdevent);
private:
  void HandleRead(base::FdEvent* fd_event) override;
  void HandleWrite(base::FdEvent* fd_event) override;
  void HandleError(base::FdEvent* fd_event) override;
  void HandleClose(base::FdEvent* fd_event) override;

  void CleanUpBadChannel(base::FdEvent* event);
  bool remove_fdevent(base::FdEvent* event);
private:
  base::EventPump* pump_;
  ConnectorDelegate* delegate_;
  std::set<base::RefFdEvent> connecting_sockets_;
  //DoubleLinkedList<FdEvent> listen_events_;
};

}}
#endif
