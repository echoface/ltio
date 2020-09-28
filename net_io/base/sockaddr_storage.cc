// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "sockaddr_storage.h"

#include <string.h>
namespace lt {
namespace net {

SockaddrStorage::SockaddrStorage()
  : addr_len(sizeof(addr_storage)) {
}

SockaddrStorage::SockaddrStorage(const SockaddrStorage& other)
  : addr_len(other.addr_len) {
    memcpy(&addr_storage, &(other.addr_storage), addr_len);
}

struct sockaddr* SockaddrStorage::AsSockAddr() {
  return reinterpret_cast<struct sockaddr*>(&addr_storage);
}

struct sockaddr_in* SockaddrStorage::AsSockAddrIn() {
  return reinterpret_cast<struct sockaddr_in*>(&addr_storage);
}
struct sockaddr_in6* SockaddrStorage::AsSockAddrIn6() {
  return reinterpret_cast<struct sockaddr_in6*>(&addr_storage);
}

void SockaddrStorage::operator=(const SockaddrStorage& other) {
  addr_len = other.addr_len;
  memcpy(&addr_storage, &(other.addr_storage), addr_len);
}

}}  // namespace net
