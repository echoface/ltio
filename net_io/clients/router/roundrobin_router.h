#ifndef _LT_NET_ROUNDROBIN_ROUTER_H_H
#define _LT_NET_ROUNDROBIN_ROUTER_H_H

#include "client_router.h"

namespace net {

class RoundRobinRouter : public ClientRouter {
  public:
    RoundRobinRouter() {};
    ~RoundRobinRouter() {};

    void StopAllClients() override;
    void AddClient(ClientPtr&& client) override;
    Client* GetNextClient(const std::string& key,
                          ProtocolMessage* request = NULL) override;
  private:
    std::vector<ClientPtr> clients_;
    std::atomic_uint32_t round_index_;
};

}
#endif
