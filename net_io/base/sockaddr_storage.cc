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
