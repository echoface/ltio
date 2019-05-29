#ifndef _LT_NET_MAGLEV_ROUTER_H_H
#define _LT_NET_MAGLEV_ROUTER_H_H

#include <cstdint>
#include <vector>
#include "maglev.h"
#include "client_router.h"

namespace net {

class MaglevRouter : ClientRouter {
  public:
    MaglevRouter();
    ~MaglevRouter();

    void StopAllClients() override;
    void AddClient(ClientPtr&& client) override;
    Client* GetNextClient(const std::string& key, ProtocolMessage* request = NULL) override;

  private:
    LookupTable lookup_table_;
    std::vector<ClientPtr> clients_;
};

} // namespace lt::net

#endif
