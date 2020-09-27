#ifndef _LT_NET_UDP_CONTEXT_H_
#define _LT_NET_UDP_CONTEXT_H_

#include <memory>
#include <base/base_micro.h>
#include <net_io/io_buffer.h>
#include "net_io/base/ip_endpoint.h"

namespace base {
  class MessageLoop;
}

namespace lt {
namespace net {

struct UDPSegment {
  IOBuffer buffer;
  IPEndPoint sender;
};

class UDPIOContext;
typedef std::unique_ptr<UDPIOContext> UDPIOContextPtr;

class UDPIOContext {
public:
  static UDPIOContextPtr Create(base::MessageLoop* io);
  ~UDPIOContext(){};

private:
  UDPIOContext(base::MessageLoop* io);

private:
  base::MessageLoop* io_;
  std::vector<UDPSegment> segments_;

  DISALLOW_COPY_AND_ASSIGN(UDPIOContext);
};


}}
#endif
