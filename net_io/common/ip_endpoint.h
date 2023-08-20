// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NET_BASE_IP_ENDPOINT_H_
#define NET_BASE_IP_ENDPOINT_H_

#include <stdint.h>
#include <cstdint>

#include <unistd.h>
#include <string>

#include "address_family.h"
#include "ip_address.h"
#include "net_export.h"
#include "sockaddr_storage.h"
#include "sys_addrinfo.h"

struct sockaddr;

namespace lt {
namespace net {

// An IPEndPoint represents the address of a transport endpoint:
//  * IP address (either v4 or v6)
//  * Port
class IPEndPoint {
public:
  ~IPEndPoint();
  IPEndPoint();
  IPEndPoint(const base::StringPiece ip, uint16_t port);
  IPEndPoint(const IPAddress& address, uint16_t port);
  IPEndPoint(const IPEndPoint& endpoint);

  uint16_t port() const { return port_; }
  const IPAddress& address() const { return address_; }

  // Returns AddressFamily of the address.
  AddressFamily GetFamily() const;

  // Returns the sockaddr family of the address, AF_INET or AF_INET6.
  int GetSockAddrFamily() const;

  // Convert to a provided sockaddr struct.
  // |address_length| is the size of data in |address| available.
  //
  // on failure: return zero
  // on success: return size of the addresss that was copied into address
  socklen_t ToSockAddr(struct sockaddr* address,
                       socklen_t address_length) const;

  // Convert from a sockaddr struct.
  // |address| is the address.
  // |address_length| is the length of |address|.
  // Returns true on success, false on failure.
  bool FromSockAddr(const struct sockaddr* address, socklen_t address_length);

  // Returns value as a string (e.g. "127.0.0.1:80"). Returns the empty string
  // when |address_| is invalid (the port will be ignored).
  std::string ToString() const;

  // As above, but without port. Returns the empty string when address_ is
  // invalid.
  std::string ToStringWithoutPort() const;

  bool operator<(const IPEndPoint& that) const;
  bool operator==(const IPEndPoint& that) const;
  bool operator!=(const IPEndPoint& that) const;

private:
  IPAddress address_;
  uint16_t port_;
};

}  // namespace net
}  // namespace lt

#endif  // NET_BASE_IP_ENDPOINT_H_
