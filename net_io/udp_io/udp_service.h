#ifndef _NET_IO_UDP_SERVICE_H_H_
#define _NET_IO_UDP_SERVICE_H_H_

#include <cstddef>
#include <memory>
#include <vector>
#include "udp_context.h"
#include "base/message_loop/fd_event.h"

namespace lt {
namespace net {

class UDPService;
typedef std::shared_ptr<UDPService> RefUDPService;

class UDPService : public EnableShared(UDPService),
                   public base::FdEvent::Handler {
public:
  class Reciever {
    public:
      virtual void OnDataRecieve(UDPIOContextPtr context) = 0;
  };

public:
  ~UDPService() {};
  static RefUDPService Create(base::MessageLoop* io,
                              const IPEndPoint& ep);

  void StartService();
  void StopService();

private:
  UDPService(base::MessageLoop* io, const IPEndPoint& ep);

  //override from FdEvent::Handler
  bool HandleRead(base::FdEvent* fd_event) override;
  bool HandleWrite(base::FdEvent* fd_event) override;
  bool HandleError(base::FdEvent* fd_event) override;
  bool HandleClose(base::FdEvent* fd_event) override;

private:
  base::MessageLoop* io_;
  IPEndPoint endpoint_;
  base::RefFdEvent socket_event_;
  DISALLOW_COPY_AND_ASSIGN(UDPService);
};

}}
#endif
