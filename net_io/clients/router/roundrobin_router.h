#ifndef _LT_NET_ROUNDROBIN_ROUTER_H_H
#define _LT_NET_ROUNDROBIN_ROUTER_H_H

#include "client_router.h"

namespace lt {
namespace net {

class RoundRobinRouter : public ClientRouter {
  public:
    RoundRobinRouter() {};
    ~RoundRobinRouter() {};

    void AddClient(RefClient&& client) override;
    RefClient GetNextClient(const std::string& key,
                            CodecMessage* request = NULL) override;
  private:
    std::vector<RefClient> clients_;
    std::atomic_uint32_t round_index_;
};

}}
#endif
