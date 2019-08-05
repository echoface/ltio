#include "address.h"

#include <strings.h>
#include <endian.h>
#include "glog/logging.h"

namespace lt {
namespace net {

SocketAddress::SocketAddress(uint16_t port) {
  bzero(&addr_in_, sizeof addr_in_);

  addr_in_.sin_family = AF_INET;

  in_addr_t ip = INADDR_ANY;//0.0.0.0

  addr_in_.sin_port = htobe16(port);
  addr_in_.sin_addr.s_addr = htobe32(ip);
}

SocketAddress::SocketAddress(const std::string& ip, const uint16_t port) {
  bzero(&addr_in_, sizeof addr_in_);
  socketutils::FromIpPort(ip.c_str(), port, &addr_in_);
}

SocketAddress::SocketAddress(const struct sockaddr_in& addr)
  : addr_in_(addr) {
}

//static
SocketAddress SocketAddress::FromSocketFd(int fd) {
  struct sockaddr_in addr = socketutils::GetLocalAddrIn(fd);
  return SocketAddress(addr);
}

const struct sockaddr* SocketAddress::AsSocketAddr() const {
  return socketutils::sockaddr_cast(&addr_in_);
}

inline uint16_t SocketAddress::Port() const {
  return be16toh(addr_in_.sin_port);
}

sa_family_t SocketAddress::Family() const {
  return addr_in_.sin_family;
}

std::string SocketAddress::Ip() const {
  return socketutils::SocketAddr2Ip(socketutils::sockaddr_cast(&addr_in_));
}

std::string SocketAddress::IpPort() const {
  return socketutils::SocketAddr2IpPort(socketutils::sockaddr_cast(&addr_in_));
}

uint32_t SocketAddress::NetworkEndianIp() const {
  return addr_in_.sin_addr.s_addr;
}

uint16_t SocketAddress::NetworkEndianPort() const {
  return addr_in_.sin_port;
}

}}
