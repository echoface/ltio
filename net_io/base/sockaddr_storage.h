#ifndef NET_BASE_SOCKADDR_STORAGE_H_
#define NET_BASE_SOCKADDR_STORAGE_H_

#include "sys_addrinfo.h"

namespace lt {
namespace net {

// Convenience struct for when you need a |struct sockaddr|.
typedef struct SockaddrStorage {
  SockaddrStorage();
  SockaddrStorage(const SockaddrStorage& other);
  void operator=(const SockaddrStorage& other);

  struct sockaddr_storage addr_storage;
  socklen_t addr_len;
  struct sockaddr* const addr;
}SockaddrStorage;

}}  // namespace net

#endif  // NET_BASE_SOCKADDR_STORAGE_H_
