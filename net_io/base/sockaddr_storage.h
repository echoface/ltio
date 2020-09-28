#ifndef NET_BASE_SOCKADDR_STORAGE_H_
#define NET_BASE_SOCKADDR_STORAGE_H_

#include "sys_addrinfo.h"
#include <cstddef>
#include <netinet/in.h>
#include <sys/socket.h>

namespace lt {
namespace net {
//sockaddr_in
// Convenience struct for when you need a |struct sockaddr|.
typedef struct SockaddrStorage {
  SockaddrStorage();
  SockaddrStorage(const SockaddrStorage& other);
  void operator=(const SockaddrStorage& other);

  struct sockaddr* AsSockAddr();
  struct sockaddr_in* AsSockAddrIn();
  struct sockaddr_in6* AsSockAddrIn6();
  const size_t Size() const {return addr_len;};

  socklen_t addr_len;
  struct sockaddr_storage addr_storage;
}SockaddrStorage;

}}  // namespace net

#endif  // NET_BASE_SOCKADDR_STORAGE_H_
