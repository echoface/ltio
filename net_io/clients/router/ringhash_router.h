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

#ifndef _LT_NET_RINGHASH_ROUTER_H_H
#define _LT_NET_RINGHASH_ROUTER_H_H

#include <zlib.h>  //crc32

#include "client_router.h"
#include "hash/murmurhash3.h"
#include "net_io/common/load_balance/consistent_hash_map.h"

namespace lt {
namespace net {

class RingHashRouter : public ClientRouter {
public:
  struct ClientNode {
    ClientNode(RefClient client, uint32_t id);
    RefClient client;
    const uint32_t vnode_id;
    const std::string hash_key;
  };

  struct NodeHasher {
    typedef uint32_t result_type;
    result_type operator()(const ClientNode& node);
  };

  typedef lb::ConsistentHashMap<ClientNode, NodeHasher> NodeContainer;

public:
  RingHashRouter(uint32_t vnode_count);
  RingHashRouter() : RingHashRouter(50){};
  ~RingHashRouter(){};

  void AddClient(RefClient&& client) override;
  RefClient GetNextClient(const std::string& key,
                          CodecMessage* request = NULL) override;

private:
  NodeContainer clients_;
  const uint32_t vnode_count_ = 50;
};

}  // namespace net
}  // namespace lt
#endif
