#include "udp_context.h"

namespace lt {
namespace net {

UDPIOContextPtr UDPIOContext::Create(base::MessageLoop* io) {
  UDPIOContextPtr context(new UDPIOContext(io));
  return context;
}

UDPIOContext::UDPIOContext(base::MessageLoop* io)
  : io_(io) {
}


}}
