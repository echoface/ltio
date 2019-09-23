#include "address.h"

#include <strings.h>
#include <endian.h>
#include "glog/logging.h"

namespace lt {
namespace net {

SocketAddr::SocketAddr(uint16_t port) {
  bzero(&addr_in_, sizeof addr_in_);

  addr_in_.sin_family = AF_INET;

  in_addr_t ip = INADDR_ANY;//0.0.0.0

  addr_in_.sin_port = htobe16(port);
  addr_in_.sin_addr.s_addr = htobe32(ip);
}

SocketAddr::SocketAddr(const std::string& ip, const uint16_t port) {
  bzero(&addr_in_, sizeof addr_in_);
  socketutils::FromIpPort(ip.c_str(), port, &addr_in_);
}

SocketAddr::SocketAddr(const struct sockaddr_in& addr)
  : addr_in_(addr) {
}

//static
SocketAddr SocketAddr::FromSocketFd(int fd) {
  struct sockaddr_in addr = socketutils::GetLocalAddrIn(fd);
  return SocketAddr(addr);
}

const struct sockaddr* SocketAddr::AsSocketAddr() const {
  return socketutils::sockaddr_cast(&addr_in_);
}

inline uint16_t SocketAddr::Port() const {
  return be16toh(addr_in_.sin_port);
}

sa_family_t SocketAddr::Family() const {
  return addr_in_.sin_family;
}

std::string SocketAddr::Ip() const {
  return socketutils::SocketAddr2Ip(socketutils::sockaddr_cast(&addr_in_));
}

std::string SocketAddr::IpPort() const {
  return socketutils::SocketAddr2IpPort(socketutils::sockaddr_cast(&addr_in_));
}

uint32_t SocketAddr::NetworkEndianIp() const {
  return addr_in_.sin_addr.s_addr;
}

uint16_t SocketAddr::NetworkEndianPort() const {
  return addr_in_.sin_port;
}

}}
