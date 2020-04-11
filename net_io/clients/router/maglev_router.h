#ifndef _LT_NET_MAGLEV_ROUTER_H_H
#define _LT_NET_MAGLEV_ROUTER_H_H

#include <cstddef>
#include <cstdint>
#include <unordered_map>
#include <vector>
#include "maglev.h"
#include "client_router.h"

namespace lt {
namespace net {

class MaglevClientEndpoint {
public:
  MaglevClientEndpoint(RefClient& client);
  uint32_t Weight() const {return node_.weight;}
  uint32_t HashValue() const {return node_.hash;}
  uint32_t IdentifyNum() const {return node_.num;}
  void SetNodeWeight(uint32_t weight) {node_.weight = weight;};
  const MaglevHelper::Endpoint& Node() const {return node_;}
  RefClient client_;
private:
  MaglevHelper::Endpoint node_;
};

class MaglevRouter : ClientRouter {
  public:
    MaglevRouter() {};
    ~MaglevRouter() {};

    void AddClient(RefClient&& client) override;

    void StartRouter() override;

    RefClient GetNextClient(const std::string& key,
                            CodecMessage* request = NULL) override;

  private:
    LookupTable lookup_table_;
    //std::vector<MaglevClientEndpoint> node_list_;
    std::unordered_map<int, MaglevClientEndpoint> nodes_;
};

}} // namespace lt::net
#endif
