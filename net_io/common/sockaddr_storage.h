/*
 * Copyright 2021 <name of copyright holder>
 * Author: Huan.Gong <gonghuan.dev@gmail.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef NET_BASE_SOCKADDR_STORAGE_H_
#define NET_BASE_SOCKADDR_STORAGE_H_

#include <netinet/in.h>
#include <sys/socket.h>
#include <cstddef>
#include "sys_addrinfo.h"

namespace lt {
namespace net {
// sockaddr_in
// Convenience struct for when you need a |struct sockaddr|.
typedef struct SockaddrStorage {
  SockaddrStorage();
  SockaddrStorage(const SockaddrStorage& other);
  void operator=(const SockaddrStorage& other);

  struct sockaddr* AsSockAddr();
  struct sockaddr_in* AsSockAddrIn();
  struct sockaddr_in6* AsSockAddrIn6();
  const size_t Size() const { return addr_len; };

  socklen_t addr_len;
  struct sockaddr_storage addr_storage;
} SockaddrStorage;

}  // namespace net
}  // namespace lt

#endif  // NET_BASE_SOCKADDR_STORAGE_H_
