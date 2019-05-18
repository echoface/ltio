#ifndef _LT_NET_RINGHASH_ROUTER_H_H
#define _LT_NET_RINGHASH_ROUTER_H_H

#include "client_router.h"
#include "consistant_hash_map.h"

namespace net {

struct ClientNode {
  std::string HashKey() const {
    return "";
  }
  ClientPtr client_;
  uint32_t vnode_id_;
};

struct crc32_hasher {
  uint32_t operator()(const ClientNode& node) {
    //return ret.checksum();
    return 0;
  }
  typedef uint32_t result_type;
};

typedef consistent_hash_map<ClientNode, crc32_hasher> CHashMap;

/*consistant hash based router*/
class RingHashRouter : public ClientRouter {
  public:
    RingHashRouter() {};
    ~RingHashRouter() {};

    void StopAllClients() override;
    void AddClient(ClientPtr&& client) override;
    Client* GetNextClient(const std::string& key,
                          ProtocolMessage* request = 0) override;
  private:
    CHashMap clients_;
};

}
#endif
