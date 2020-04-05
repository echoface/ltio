#ifndef _LT_NET_RINGHASH_ROUTER_H_H
#define _LT_NET_RINGHASH_ROUTER_H_H

#include <zlib.h> //crc32
#include "chash_map.h"
#include "client_router.h"
#include "thirdparty/murmurhash/MurmurHash3.h"

namespace lt {
namespace net {

struct ClientNode {
  ClientNode(RefClient client, uint32_t id)
    : client(client),
      vnode_id(id),
      hash_key(client->RemoteIpPort() + std::to_string(id)) {
    //LOG(INFO) << "hash_key:" << hash
    //<< " crc32:" << ::crc32(0x80000000, (const unsigned char*)hash.data(), hash.size());
  }
  ~ClientNode() {};
  RefClient client;
  const uint32_t vnode_id;
  const std::string hash_key;
};

struct crc32_hasher {
  uint32_t operator()(const ClientNode& node) {
    uint32_t out = 0;
    MurmurHash3_x86_32(node.hash_key.data(),
                       node.hash_key.size(),
                       0x80000000,
                       &out);
    return out;
    //return ::crc32(0x80000000, (const unsigned char*)node.hash.data(), node.hash.size());
  }
  typedef uint32_t result_type;
};

typedef CHashMap<ClientNode, crc32_hasher> NodeContainer;

/*consistant hash based router*/
class RingHashRouter : public ClientRouter {
  public:
    RingHashRouter() {};
    RingHashRouter(uint32_t vnode_count);
    ~RingHashRouter() {};

    void AddClient(RefClient&& client) override;
    RefClient GetNextClient(const std::string& key,
                            ProtocolMessage* request = NULL) override;
  private:
    NodeContainer clients_;
    std::list<RefClient> all_clients_;
    const uint32_t vnode_count_ = 50;
};

}} //lt::net
#endif
