#ifndef _LT_NET_CLIENT_CONNECTOR_H_H
#define _LT_NET_CLIENT_CONNECTOR_H_H

#include <atomic>
#include <bits/stdint-uintn.h>
#include <cstdint>
#include <list>
#include <set>
#include <sys/types.h>

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
using base::FdEvent;
using base::RefFdEvent;
using base::EventPump;

typedef std::shared_ptr<Connector> RefConnector;
typedef std::unique_ptr<Connector> OwnedConnector;
typedef std::weak_ptr<base::FdEvent> WeakPtrFdEvent;


class Connector : public base::FdEvent::Handler {
public:
  class Delegate {
    public:
      virtual ~Delegate() {};
      //notify connect failed and report inprogress count
      virtual void OnConnectFailed(uint32_t count) = 0;
      virtual void OnConnected(int socket_fd, IPEndPoint& local, IPEndPoint& remote) = 0;
  };
  Connector(base::EventPump* pump, Delegate* delegate);
  ~Connector() {};

  void Stop();

  /*gurantee a callback atonce or later*/
  void Launch(const net::IPEndPoint &address);

  uint32_t InprocessCount() const {return count_;}
private:
  bool HandleRead(FdEvent* fd_event) override;
  bool HandleWrite(FdEvent* fd_event) override;
  bool HandleError(FdEvent* fd_event) override;
  bool HandleClose(FdEvent* fd_event) override;

  void Cleanup(base::FdEvent* event);
private:
  EventPump* pump_;
  Delegate* delegate_;
  std::atomic_uint32_t count_;
  std::list<RefFdEvent> inprogress_list_;
};

}}
#endif
