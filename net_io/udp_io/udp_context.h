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

#ifndef _LT_NET_UDP_CONTEXT_H_
#define _LT_NET_UDP_CONTEXT_H_

#include <base/base_micro.h>
#include <net_io/io_buffer.h>
#include <memory>
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

}  // namespace net
}  // namespace lt
#endif
