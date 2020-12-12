#ifndef _LT_NET_MAGLEV_ROUTER_H_H
#define _LT_NET_MAGLEV_ROUTER_H_H

#include <cstddef>
#include <cstdint>
#include <unordered_map>
#include <vector>

#include "client_router.h"
#include "net_io/base/load_balance/maglev2.h"

namespace lt {
namespace net {

class MaglevRouter : ClientRouter {
  public:
    MaglevRouter() {};
    ~MaglevRouter() {};

    void AddClient(RefClient&& client) override;

    void StartRouter() override;

    RefClient GetNextClient(const std::string& key,
                            CodecMessage* request = NULL) override;

  private:
    lb::LookupTable lookup_table_;
    std::vector<RefClient> clients_;
};

}} // namespace lt::net
#endif
