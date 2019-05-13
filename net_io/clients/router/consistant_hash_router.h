#ifndef _LT_NET_CONSISTANTHASH_ROUTER_H_H
#define _LT_NET_CONSISTANTHASH_ROUTER_H_H

#include "client_router.h"
#include "consistant_hash_router.h"

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
    return ret.checksum();
  }
  typedef uint32_t result_type;
};

/*consistant hash based router*/
class ConHashRouter : public ClientRouter {
  public:
    ConHashRouter() {};
    ~ConHashRouter() {};

    void StopAllClients() override;
    void AddClient(ClientPtr&& client) override;
    Client* GetNextClient(const std::string& key,
                          ProtocolMessage* request = 0) override;
  private:
    std::vector<ClientPtr> clients_;
    consistent_hash_map<std::string, 
};

}
#endif
